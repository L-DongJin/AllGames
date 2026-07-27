# AllGames Windows 패키징 가이드

## 1. 패키징 전에 확인할 것

1. Unreal Editor에서 수정한 맵과 Blueprint를 모두 저장한다.
2. Unreal Editor를 완전히 종료한다. C++ 전체 빌드와 동시에 Editor 또는 Live Coding을 실행하지 않는다.
3. 프로젝트 여유 공간을 확인한다. 현재 AllGames는 Cook/Stage/ZIP을 함께 만들 때 최소 8-10GB의 여유 공간을 권장한다.
4. 인터넷 멀티플레이 배포본이라면 `Config/DefaultEngine.ini`의 EOS 설정이 패키지에 필요한 공개 식별자만 포함하는지 확인한다. Client Secret과 같은 비밀 값은 절대 넣지 않는다.
5. 실제 배포 권리가 없는 음악과 인물 이미지는 공개 배포본에 포함하지 않는다.

## 2. 현재 프로젝트의 권장 Packaging 설정

Unreal Editor에서 `Edit > Project Settings > Project > Packaging`을 연다.

- Build Configuration: `Development` (친구 테스트용)
- Full Rebuild: 켜기
- For Distribution: 끄기
- Use Pak File: 켜기
- Use Io Store: 켜기
- Create compressed cooked packages: 켜기
- Include Prerequisites Installer: 켜기
- Include App Local Prerequisites: 켜기
- App-local prerequisites directory:
  `$(EngineDir)/Binaries/ThirdParty/AppLocalDependencies`
- Include Debug Files: 끄기
- Include Crash Reporter: 현재는 끄기
- Cook only maps: 켜기

현재 `Config/DefaultGame.ini`에는 다음 맵이 명시적으로 Cook된다.

- `LobbyMap`
- `FiveKeyMap`
- `MainHubMap`
- `IdolQuizMap`
- `IdolQuizRoomMap`
- `IdolQuizLobbyMap`
- `DrawingQuizMap`

또한 `/Game/UI`, `/Game/Common`, `/Game/IdolQuiz`은 코드의 Soft Class 경로만으로 누락되지 않도록 항상 Cook한다.

## 3. Unreal Editor에서 패키징하는 방법

1. 프로젝트를 연다.
2. 모든 에셋을 저장한다: `File > Save All`.
3. 상단 메뉴에서 `Platforms > Windows`를 연다.
4. Binary Configuration이 `Development`인지 확인한다.
5. `Package Project`를 선택한다.
6. 새 출력 폴더를 선택한다. 예:
   `C:\GitHub\AllGames\Builds\ManualPackage_YYYYMMDD`
7. 오른쪽 아래 Output Log 또는 별도 Packaging 창에서 진행 상황을 확인한다.
8. 마지막에 `Packaging Complete`가 표시되어야 성공이다.

출력 폴더 안의 최상위 `AllGames.exe`를 실행해야 한다. `Engine`이나 `AllGames` 하위 폴더만 따로 전달하면 실행되지 않는다.

## 4. 명령줄에서 재현 가능한 전체 패키징

Editor를 종료한 후 PowerShell에서 다음 명령을 실행한다. 날짜가 바뀌면 `archivedirectory` 폴더명만 바꾼다.

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat' `
  BuildCookRun `
  -project='C:\GitHub\AllGames\AllGames.uproject' `
  -noP4 `
  -platform=Win64 `
  -clientconfig=Development `
  -build `
  -cook `
  -stage `
  -pak `
  -iostore `
  -archive `
  -archivedirectory='C:\GitHub\AllGames\Builds\EOS_Multiplayer_AppLocal_YYYYMMDD' `
  -prereqs `
  -applocaldirectory='C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\ThirdParty\AppLocalDependencies' `
  -utf8output
```

단계의 의미:

- `build`: 실제 게임용 `AllGames.exe`를 컴파일한다.
- `cook`: Unreal 에셋을 Windows 실행 형식으로 변환한다.
- `stage`: 실행 파일, 플러그인, DLL, Cook 데이터의 배포 폴더를 조립한다.
- `pak` / `iostore`: 콘텐츠를 `.pak`, `.utoc`, `.ucas` 컨테이너로 묶는다.
- `archive`: 완성된 Stage를 지정한 Builds 폴더로 복사한다.
- `prereqs`: Unreal prerequisite 설치 프로그램을 포함한다.
- `applocaldirectory`: VC++ DLL을 실행 파일 옆에도 포함하여 다른 PC의 별도 설치 요구를 줄인다.

성공 기준은 마지막 로그의 다음 두 줄이다.

```text
BUILD SUCCESSFUL
AutomationTool exiting with ExitCode=0 (Success)
```

## 5. ZIP 만드는 방법

패키지 폴더 안의 `Windows` 폴더 전체를 압축해야 한다. `AllGames.exe`만 압축하면 안 된다.

```powershell
tar.exe -a -c `
  -f 'C:\GitHub\AllGames\Builds\AllGames_EOS_Multiplayer_Latest.zip' `
  -C 'C:\GitHub\AllGames\Builds\EOS_Multiplayer_AppLocal_YYYYMMDD' `
  Windows
```

Google Drive Desktop의 드라이브가 연결된 경우 완성된 ZIP을 Drive 경로로 복사한다.

```powershell
Copy-Item `
  'C:\GitHub\AllGames\Builds\AllGames_EOS_Multiplayer_Latest.zip' `
  'G:\내 드라이브\AllGames_EOS_Multiplayer_Latest.zip' `
  -Force
```

Drive가 탐색기에서 보이지 않으면 Google Drive Desktop이 종료되었거나 드라이브 문자가 달라진 것이므로 먼저 앱을 실행하고 실제 경로를 확인한다.

## 6. 전달하기 전 필수 테스트

1. ZIP을 프로젝트 밖의 임시 폴더에 직접 풀어본다.
2. 최상위 `Windows/AllGames.exe`를 실행한다.
3. 로그인 → 게임 선택 → 퀴즈 방 목록까지 이동한다.
4. 방 생성 후 입력한 방 제목이 공용 대기실에 표시되는지 확인한다.
5. 혼자 방을 나간 뒤 새로고침하여 빈 방이 사라지는지 확인한다.
6. 서로 다른 PC와 계정으로 같은 방을 검색하고 입장한다.
7. 방장에게만 Start가 표시되는지 확인한다.
8. 인물 퀴즈와 그림 퀴즈를 각각 시작한다.
9. 리듬게임에서 음악, 입력, 판정, 결과와 온라인 점수 제출을 확인한다.

## 7. 자주 발생하는 오류

### 다른 PC에서 VC++ 설치를 요구함

- `IncludeAppLocalPrerequisites=True`인지 확인한다.
- 압축 파일 안에 `vcruntime140.dll`, `vcruntime140_1.dll`, `msvcp140.dll`이 있는지 확인한다.
- 실행 파일만 전달하지 말고 Windows 폴더 전체를 전달한다.

### 에디터에서는 UI가 나오지만 패키지에서는 안 나옴

- Blueprint가 `DirectoriesToAlwaysCook` 또는 Cook되는 맵/에셋에서 참조되는지 확인한다.
- AllGames는 `/Game/UI`를 항상 Cook하도록 설정되어 있다.

### 방 검색 또는 입장이 안 됨

- 두 PC에서 서로 다른 EOS 계정으로 로그인한다.
- 방장이 방과 게임을 실행한 상태인지 확인한다.
- 양쪽 Windows 방화벽에서 AllGames의 네트워크 접근을 허용한다.
- `Saved/Logs/AllGames.log`에서 `EOS lobby`와 `NetworkFailure`를 검색한다.

### 패키징이 실패함

- 로그 마지막의 `Error:`보다 먼저 나온 첫 번째 실제 컴파일/Cook 오류를 찾는다.
- Editor와 Rider의 빌드를 동시에 실행하지 않는다.
- `Saved`, `Intermediate`, 이전 패키지를 정리하고 다시 시도할 수 있지만 `Content`, `Config`, `Source`, `Plugins`는 삭제하면 안 된다.
