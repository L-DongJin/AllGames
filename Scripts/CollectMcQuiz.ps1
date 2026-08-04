param(
    [string]$DatasetRoot = "SourceAssets/IdolQuiz/MC",
    [int]$ExpectedCandidates = 64,
    [int]$CheckpointSize = 10,
    [int]$DelayMilliseconds = 750
)

$ErrorActionPreference = "Stop"
$sourcePage = "https://www.piku.co.kr/w/9nhamV"
$rankingPage = "https://www.piku.co.kr/w/rank/9nhamV"
$rankingEndpoint = "https://www.piku.co.kr/w/rank/x.php?u=9nhamV"
$resolvedRoot = [IO.Path]::GetFullPath((Join-Path (Get-Location) $DatasetRoot))
$imageRoot = Join-Path $resolvedRoot "Images"
$manifestPath = Join-Path $resolvedRoot "collection_manifest.json"
$exclusionPath = Join-Path $resolvedRoot "excluded_names.json"
$progressPath = Join-Path $resolvedRoot "progress.json"
$responsePath = Join-Path $resolvedRoot "ranking_response.json"

New-Item -ItemType Directory -Path $imageRoot -Force | Out-Null

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

function Get-CleanName {
    param([string] $Value)

    $cleaned = [regex]::Replace($Value, "\[[^\]]*\]|<[^>]*>", " ")
    $cleaned = [regex]::Replace($cleaned, "\s+", " ").Trim()
    return $cleaned
}

function Get-PlainText {
    param([string] $Html)

    $decoded = [System.Net.WebUtility]::HtmlDecode($Html)
    return ([regex]::Replace(([regex]::Replace($decoded, "<[^>]*>", " ")), "\s+", " ")).Trim()
}

function Save-Progress {
    param(
        [Parameter(Mandatory)] $Manifest,
        [Parameter(Mandatory)] [string] $Status
    )

    $downloaded = @($Manifest.records | Where-Object status -eq "downloaded").Count
    $failed = @($Manifest.records | Where-Object status -eq "failed").Count
    $progress = [ordered]@{
        source_page = $sourcePage
        expected_candidates = $ExpectedCandidates
        captured_candidates = @($Manifest.records).Count
        downloaded_candidates = $downloaded
        failed_candidates = $failed
        last_completed_batch = $Manifest.last_completed_batch
        status = $Status
        updated_at = [DateTime]::UtcNow.ToString("o")
    }
    Write-JsonAtomic -Value $Manifest -Path $manifestPath
    Write-JsonAtomic -Value $progress -Path $progressPath
}

if (Test-Path -LiteralPath $manifestPath) {
    $manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath | ConvertFrom-Json
}
else {
    & curl.exe --http1.1 -L --fail --silent --show-error `
        --max-time 15 --max-filesize 1048576 `
        -A "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" `
        -H "Referer: $rankingPage" `
        --data-urlencode "draw=1" `
        --data-urlencode "start=0" `
        --data-urlencode "length=$ExpectedCandidates" `
        --data-urlencode "search[value]=" `
        --data-urlencode "order[0][column]=0" `
        --data-urlencode "order[0][dir]=asc" `
        $rankingEndpoint -o $responsePath
    if ($LASTEXITCODE -ne 0) {
        throw "Ranking request failed with curl exit code $LASTEXITCODE"
    }

    $responseText = [Text.Encoding]::UTF8.GetString([IO.File]::ReadAllBytes($responsePath))
    $response = $responseText | ConvertFrom-Json
    $rows = @($response.data)
    if ([int]$response.recordsTotal -ne $ExpectedCandidates -or $rows.Count -ne $ExpectedCandidates) {
        throw "Expected $ExpectedCandidates candidates but received total=$($response.recordsTotal), rows=$($rows.Count)"
    }

    $records = [System.Collections.Generic.List[object]]::new()
    for ($index = 0; $index -lt $rows.Count; $index++) {
        $row = $rows[$index]
        $nameMatch = [regex]::Match([string]$row[2], "<strong[^>]*>(.*?)</strong>", "Singleline")
        $originalName = if ($nameMatch.Success) { Get-PlainText $nameMatch.Groups[1].Value } else { Get-PlainText ([string]$row[2]) }
        $cleanName = Get-CleanName $originalName
        $urlMatch = [regex]::Match(([string]$row[1]), "https?://[^`"'`\s<>]+")
        if (-not $cleanName -or -not $urlMatch.Success) {
            throw "Candidate metadata is incomplete at index $index"
        }

        $imageUrl = $urlMatch.Value.TrimEnd(")")
        $records.Add([ordered]@{
            index = $index + 1
            original_name = $originalName
            answer = $cleanName
            image_url = $imageUrl
            saved_filename = (([regex]::Replace(([regex]::Replace($cleanName, '[<>:"/\\|?*]', ' - ')), '\s+', ' ').Trim().TrimEnd('.')) + '.png')
            status = "pending"
            width = $null
            height = $null
            size_bytes = $null
            sha256 = $null
            error = $null
        })
    }

    $manifest = [pscustomobject][ordered]@{
        source_page = $sourcePage
        expected_candidates = $ExpectedCandidates
        batch_size = $CheckpointSize
        last_completed_batch = 0
        records = @($records)
    }
    Save-Progress -Manifest $manifest -Status "metadata_captured"
}

$excludedNames = @()
if (Test-Path -LiteralPath $exclusionPath) {
    $excludedNames = @(Get-Content -Raw -Encoding UTF8 -LiteralPath $exclusionPath | ConvertFrom-Json)
}

for ($index = 0; $index -lt $manifest.records.Count; $index++) {
    $record = $manifest.records[$index]
    $target = Join-Path $imageRoot $record.saved_filename

    if ($excludedNames -contains $record.answer) {
        continue
    }

    if ($record.status -eq "downloaded" -and (Test-Path -LiteralPath $target)) {
        continue
    }

    $temporaryDownload = "$target.download"
    try {
        & curl.exe --http1.1 -L --fail --silent --show-error `
            --max-time 60 --max-filesize 10485760 `
            -A "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" `
            -H "Referer: https://www.piku.co.kr/" `
            $record.image_url -o $temporaryDownload
        if ($LASTEXITCODE -ne 0) {
            throw "Image request failed with curl exit code $LASTEXITCODE"
        }

        Add-Type -AssemblyName System.Drawing
        $sourceImage = [System.Drawing.Image]::FromFile($temporaryDownload)
        try {
            $sourceImage.Save($target, [System.Drawing.Imaging.ImageFormat]::Png)
        }
        finally {
            $sourceImage.Dispose()
        }

        $verifiedImage = [System.Drawing.Image]::FromFile($target)
        try {
            if ($verifiedImage.Width -le 0 -or $verifiedImage.Height -le 0) {
                throw "Converted image has invalid dimensions"
            }
            $record.width = $verifiedImage.Width
            $record.height = $verifiedImage.Height
        }
        finally {
            $verifiedImage.Dispose()
        }

        $file = Get-Item -LiteralPath $target
        if ($file.Length -le 0) {
            throw "Converted image is empty"
        }
        $record.size_bytes = $file.Length
        $record.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash.ToLowerInvariant()
        $record.status = "downloaded"
        $record.error = $null
    }
    catch {
        $record.status = "failed"
        $record.error = $_.Exception.Message
    }
    finally {
        Remove-Item -LiteralPath $temporaryDownload -ErrorAction SilentlyContinue
    }

    $completed = $index + 1
    if (($completed % $CheckpointSize) -eq 0 -or $completed -eq $manifest.records.Count) {
        $manifest.last_completed_batch = [int][Math]::Ceiling($completed / $CheckpointSize)
        Save-Progress -Manifest $manifest -Status "batch_$('{0:D3}' -f ([int]$manifest.last_completed_batch))_checkpoint"
    }

    if ($completed -lt $manifest.records.Count) {
        Start-Sleep -Milliseconds $DelayMilliseconds
    }
}

$failures = @($manifest.records | Where-Object status -eq "failed")
$finalStatus = if ($failures.Count -eq 0) { "complete" } else { "download_failures_pending" }
Save-Progress -Manifest $manifest -Status $finalStatus

$downloadedCount = @($manifest.records | Where-Object status -eq "downloaded").Count
Write-Output "captured=$($manifest.records.Count) downloaded=$downloadedCount failed=$($failures.Count) status=$finalStatus"
