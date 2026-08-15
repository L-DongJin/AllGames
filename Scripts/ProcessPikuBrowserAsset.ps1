param(
    [Parameter(Mandatory)] [string] $BrowserAssetPath,
    [int] $RecordIndex = 0,
    [string] $SourceUrl,
    [string] $DatasetRoot = "SourceAssets/IdolQuiz/AfreecaBJ",
    [int] $ExpectedCandidates = 64,
    [int] $CheckpointSize = 10
)

$ErrorActionPreference = "Stop"
$resolvedRoot = [IO.Path]::GetFullPath((Join-Path (Get-Location) $DatasetRoot))
$imageRoot = Join-Path $resolvedRoot "Images"
$metadataPath = Join-Path $resolvedRoot "browser_metadata.json"
$manifestPath = Join-Path $resolvedRoot "collection_manifest.json"
$progressPath = Join-Path $resolvedRoot "progress.json"

function Write-JsonAtomic {
    param(
        [Parameter(Mandatory)] $Value,
        [Parameter(Mandatory)] [string] $Path,
        [int] $Depth = 8
    )
    $temporaryPath = "$Path.tmp"
    $Value | ConvertTo-Json -Depth $Depth | Set-Content -LiteralPath $temporaryPath -Encoding UTF8
    Move-Item -LiteralPath $temporaryPath -Destination $Path -Force
}

function Get-SafeFilename {
    param([Parameter(Mandatory)] [string] $Name)
    $safe = [regex]::Replace($Name, '[<>:"/\\|?*]', ' - ')
    $safe = [regex]::Replace($safe, '\s+', ' ').Trim().TrimEnd('.')
    if (-not $safe) { throw "Empty safe filename for '$Name'" }
    return "$safe.png"
}

if (-not (Test-Path -LiteralPath $metadataPath -PathType Leaf)) {
    throw "Missing browser metadata: $metadataPath"
}
$metadata = Get-Content -LiteralPath $metadataPath -Raw -Encoding UTF8 | ConvertFrom-Json
if (@($metadata.records).Count -ne $ExpectedCandidates) {
    throw "Expected $ExpectedCandidates metadata records, found $(@($metadata.records).Count)"
}
if ($SourceUrl) {
    $sourceMatches = @($metadata.records | Where-Object { [string]$_.image_url -eq $SourceUrl })
    if ($sourceMatches.Count -ne 1) {
        throw "Expected one metadata record for source URL, found $($sourceMatches.Count): $SourceUrl"
    }
    $RecordIndex = [int]$sourceMatches[0].index
}
elseif ($RecordIndex -le 0) {
    throw "Provide either -SourceUrl or a positive -RecordIndex"
}

if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
    $manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
}
else {
    $records = foreach ($sourceRecord in $metadata.records) {
        [pscustomobject][ordered]@{
            index = [int]$sourceRecord.index
            original_name = [string]$sourceRecord.name
            answer = [string]$sourceRecord.name
            image_url = [string]$sourceRecord.image_url
            saved_filename = (Get-SafeFilename -Name ([string]$sourceRecord.name))
            status = "pending"
            width = $null
            height = $null
            size_bytes = $null
            sha256 = $null
            error = $null
        }
    }
    $duplicates = @($records | Group-Object saved_filename | Where-Object Count -gt 1)
    if ($duplicates.Count -gt 0) {
        throw "Filename collisions detected: $($duplicates.Name -join ', ')"
    }
    $manifest = [pscustomobject][ordered]@{
        source_page = [string]$metadata.source_page
        source_rank_page = [string]$metadata.source_rank_page
        collection_method = "chrome_visible_rank_table_and_page_assets"
        expected_candidates = $ExpectedCandidates
        batch_size = $CheckpointSize
        last_completed_batch = 0
        records = @($records)
    }
}

$record = @($manifest.records | Where-Object { [int]$_.index -eq $RecordIndex })
if ($record.Count -ne 1) { throw "Expected one record for index $RecordIndex, found $($record.Count)" }
$record = $record[0]
$sourcePath = [IO.Path]::GetFullPath($BrowserAssetPath)
if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) { throw "Missing browser asset: $sourcePath" }
$sourceItem = Get-Item -LiteralPath $sourcePath
if ($sourceItem.Length -le 0 -or $sourceItem.Length -gt 10MB) { throw "Invalid source size: $($sourceItem.Length)" }

New-Item -ItemType Directory -Path $imageRoot -Force | Out-Null
$targetPath = Join-Path $imageRoot $record.saved_filename
$temporaryPng = "$targetPath.tmp.png"
Add-Type -AssemblyName System.Drawing
$sourceImage = [System.Drawing.Image]::FromFile($sourcePath)
try {
    if ($sourceImage.Width -le 0 -or $sourceImage.Height -le 0) { throw "Invalid source dimensions" }
    $sourceImage.Save($temporaryPng, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    $sourceImage.Dispose()
}

$verified = [System.Drawing.Image]::FromFile($temporaryPng)
try {
    if ($verified.Width -le 0 -or $verified.Height -le 0) { throw "Invalid PNG dimensions" }
    $width = $verified.Width
    $height = $verified.Height
    if ($verified.RawFormat.Guid -ne [System.Drawing.Imaging.ImageFormat]::Png.Guid) {
        throw "Converted file is not PNG"
    }
}
finally {
    $verified.Dispose()
}
if (Test-Path -LiteralPath $targetPath -PathType Leaf) {
    [IO.File]::Copy($temporaryPng, $targetPath, $true)
    Remove-Item -LiteralPath $temporaryPng
}
else {
    Move-Item -LiteralPath $temporaryPng -Destination $targetPath
}
$record.status = "downloaded"
$record.width = $width
$record.height = $height
$record.size_bytes = (Get-Item -LiteralPath $targetPath).Length
$record.sha256 = (Get-FileHash -LiteralPath $targetPath -Algorithm SHA256).Hash.ToLowerInvariant()
$record.error = $null

$downloaded = @($manifest.records | Where-Object status -eq "downloaded").Count
$manifest.last_completed_batch = [int][Math]::Floor($downloaded / $CheckpointSize)
$status = if ($downloaded -eq $ExpectedCandidates) { "complete" } else { "browser_collection_in_progress" }
$progress = [ordered]@{
    source_page = $manifest.source_page
    expected_candidates = $ExpectedCandidates
    captured_candidates = @($manifest.records).Count
    downloaded_candidates = $downloaded
    failed_candidates = @($manifest.records | Where-Object status -eq "failed").Count
    last_completed_batch = $manifest.last_completed_batch
    status = $status
    updated_at = [DateTime]::UtcNow.ToString("o")
}
Write-JsonAtomic -Value $manifest -Path $manifestPath
Write-JsonAtomic -Value $progress -Path $progressPath
Write-Output "index=$RecordIndex name=$($record.answer) downloaded=$downloaded/$ExpectedCandidates width=$width height=$height sha256=$($record.sha256)"
