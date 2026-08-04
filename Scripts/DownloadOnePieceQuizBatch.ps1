param(
    [string]$ManifestPath = "SourceAssets/IdolQuiz/OnePiece/collection_manifest.json"
)

$ErrorActionPreference = "Stop"
$resolvedManifest = (Resolve-Path -LiteralPath $ManifestPath).Path
$datasetRoot = Split-Path -Parent $resolvedManifest
$imageRoot = Join-Path $datasetRoot "Images"
New-Item -ItemType Directory -Path $imageRoot -Force | Out-Null

$manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedManifest | ConvertFrom-Json
$failures = [System.Collections.Generic.List[object]]::new()

foreach ($record in $manifest.records) {
    if ($record.status -eq "downloaded" -and (Test-Path -LiteralPath (Join-Path $imageRoot $record.saved_filename))) {
        continue
    }

    $target = Join-Path $imageRoot $record.saved_filename
    try {
        if (-not (Test-Path -LiteralPath $target)) {
            $downloadTarget = "$target.download"
            & curl.exe --http1.1 -L --fail --silent --show-error `
                -A "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/138.0 Safari/537.36" `
                -H "Referer: https://www.piku.co.kr/" `
                -H "Accept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8" `
                -H "Sec-Fetch-Site: same-site" `
                -H "Sec-Fetch-Mode: no-cors" `
                -H "Sec-Fetch-Dest: image" `
                $record.image_url -o $downloadTarget
            if ($LASTEXITCODE -ne 0) {
                throw "curl failed with exit code $LASTEXITCODE"
            }
            Add-Type -AssemblyName System.Drawing
            $downloadedImage = [System.Drawing.Image]::FromFile($downloadTarget)
            try {
                $downloadedImage.Save($target, [System.Drawing.Imaging.ImageFormat]::Png)
            }
            finally {
                $downloadedImage.Dispose()
                Remove-Item -LiteralPath $downloadTarget -ErrorAction SilentlyContinue
            }
        }        $file = Get-Item -LiteralPath $target
        if ($file.Length -le 0) {
            throw "Downloaded file is empty"
        }

        Add-Type -AssemblyName System.Drawing
        $image = [System.Drawing.Image]::FromFile($target)
        try {
            if ($image.Width -le 0 -or $image.Height -le 0) {
                throw "Image dimensions are invalid"
            }
            $record | Add-Member -NotePropertyName width -NotePropertyValue $image.Width -Force
            $record | Add-Member -NotePropertyName height -NotePropertyValue $image.Height -Force
        }
        finally {
            $image.Dispose()
        }

        $record | Add-Member -NotePropertyName size_bytes -NotePropertyValue $file.Length -Force
        $record | Add-Member -NotePropertyName sha256 -NotePropertyValue (Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash.ToLowerInvariant() -Force
        $record.status = "downloaded"
    }
    catch {
        $record.status = "failed"
        $failures.Add([pscustomobject]@{
            piku_id = $record.piku_id
            image_url = $record.image_url
            saved_filename = $record.saved_filename
            error = $_.Exception.Message
        })
    }
}

$hashOwners = @{}
foreach ($record in $manifest.records) {
    if ($record.status -ne "downloaded" -or -not $record.sha256) {
        continue
    }
    if ($hashOwners.ContainsKey($record.sha256)) {
        $record | Add-Member -NotePropertyName duplicate_image_of -NotePropertyValue $hashOwners[$record.sha256] -Force
    }
    else {
        $hashOwners[$record.sha256] = $record.piku_id
        $record | Add-Member -NotePropertyName duplicate_image_of -NotePropertyValue $null -Force
    }
}

$manifest.failed_downloads = @($failures)
if ($failures.Count -eq 0) {
    $manifest.last_completed_batch = [Math]::Ceiling($manifest.records.Count / $manifest.batch_size)
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -LiteralPath $resolvedManifest

$downloaded = @($manifest.records | Where-Object status -eq "downloaded").Count
$progress = [ordered]@{
    source_page = $manifest.source_page
    expected_candidates = $manifest.expected_candidates
    captured_candidates = $manifest.records.Count
    downloaded_candidates = $downloaded
    failed_candidates = $failures.Count
    last_completed_batch = $manifest.last_completed_batch
    status = if ($failures.Count -eq 0) { "batch_$(([int]$manifest.last_completed_batch).ToString('000'))_complete" } else { "download_failures_pending" }
}
$progress | ConvertTo-Json | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $datasetRoot "progress.json")

Write-Output "captured=$($manifest.records.Count) downloaded=$downloaded failed=$($failures.Count)"
