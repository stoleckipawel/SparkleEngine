# Eskadra Bielika - przygotowanie maszyny przed zajeciami

Status: osobista checklista przygotowawcza  
Date: 2026-06-12  
Scope: Windows laptop, konta, logowania, narzedzia lokalne, Google Cloud, Google ADK, Bielik AI, RAG/MCP readiness

## Cel

Ten dokument ma pomoc upewnic sie przed zajeciami Eskadry Bielika, ze laptop jest gotowy do pracy hands-on, a potrzebne konta sa zalogowane i zweryfikowane.

Publiczne materialy warsztatowe wskazuja, ze zajecia moga obejmowac:

- Bielik AI jako polski model jezykowy.
- Google ADK, czyli Agent Development Kit.
- Google Cloud, Cloud Run, Cloud Shell i czasem BigQuery.
- Python, Git, prace w terminalu i uruchamianie agentow.
- W nowszej odslonie: RAG, dane, embeddingi, prosty UI, integracje systemowe i podejscie MCP.

Najwazniejszy cel praktyczny: przyjsc z laptopem, na ktorym dziala Chrome, Git, Python, terminal oraz dostep do kont Google/Google Cloud.

## Zrodla I Kontekst

- Oficjalna strona Bielik AI: https://bielik.ai/
- Projekt Eskadra Bielika na stronie Bielik AI: https://eskadra.bielik.ai/
- Repo warsztatowe Misja 1: https://github.com/speakleash/eskadra-bielik-misja1
- Google Cloud Console: https://console.cloud.google.com/
- Google AI Studio: https://aistudio.google.com/
- Google ADK docs: https://google.github.io/adk-docs/
- Przyklad wydarzenia Luma wskazuje wymagania: laptop, konto Google, Chrome, Git.
- Opis nowszych warsztatow UW wskazuje prace z RAG, GCP, BigQuery, UI, agentami i MCP.

## Status Na Tej Maszynie

Sprawdzone 2026-06-12:

- Python 3.13.14 jest zainstalowany.
- `pip 26.1.2` jest dostepny przez `python -m pip`.
- `python` powinien wskazywac na `C:\Users\stole\AppData\Local\Programs\Python\Python313\python.exe` po otwarciu nowego terminala.
- Git 2.54.0 jest zainstalowany.
- VS Code 1.123.0 jest zainstalowany.
- Google Cloud SDK 572.0.0 jest zainstalowany, razem z `bq 2.1.32` i `gsutil 5.37`.
- PowerShell ma ustawione `RemoteSigned` dla `CurrentUser`, zeby lokalne skrypty typu `Activate.ps1` i `gcloud.ps1` mogly dzialac.
- Repo warsztatowe jest sklonowane w krotkiej sciezce `C:\EB\m1`, zeby uniknac limitu dlugosci sciezek Windows.
- Virtualenv dla `adk-agents` jest gotowy w `C:\EB\m1\adk-agents\.venv`.
- Zaleznosci `google-adk 1.19.0` i `litellm 1.88.1` sa zainstalowane w tym virtualenv.
- Komenda `adk --help` dziala z virtualenv.
- Agenci z repo warsztatowego laduja sie lokalnie: `content_creator_agent` i `culinary_guide_agent`.
- Lokalny ADK Web startuje i odpowiada HTTP 200 na `http://127.0.0.1:8765`.
- `GOOGLE_API_KEY` jest wpisany do `C:\EB\m1\.env`.
- Test `ListModels` dla Gemini API dziala, co potwierdza, ze klucz/projekt sa rozpoznawane.
- Test `generateContent` dla `gemini-2.0-flash` zwraca `429 RESOURCE_EXHAUSTED`, bo free-tier quota dla tego projektu/modelu wynosi `0`. To nie jest problem instalacji lokalnej.
- Lokalny `gcloud auth login` jest gotowy dla konta `stoleckipawel98@gmail.com`.
- Lokalny `gcloud auth application-default login` jest gotowy; ADC zapisane w profilu uzytkownika.
- Aktywny projekt `gcloud` ustawiony na `project-4efc201e-a329-4966-822` (`My First Project`).
- ADC quota project ustawiony na `project-4efc201e-a329-4966-822`.
- Proba wlaczenia Cloud Run/Cloud Build/Artifact Registry/Compute API zostala zablokowana, bo billing account projektu nie jest otwarty.
- `gcloud billing accounts list` pokazuje billing accounts jako `OPEN=False`. To wymaga decyzji/uaktywnienia billing/free trial albo kredytow OnRamp przez uzytkownika/prowadzacego.

Do sprawdzenia recznie w nowym PowerShellu:

```powershell
python --version
python -m pip --version
where.exe python
```

Oczekiwane minimum:

- `python --version` pokazuje `Python 3.13.14` albo inna realna wersje CPython, nie Microsoft Store alias.
- `where.exe python` pokazuje najpierw katalog `AppData\Local\Programs\Python\Python313`.

## Checklista Krytyczna

Zrob to najpozniej dzien przed zajeciami.

| Obszar | Co przygotowac | Jak sprawdzic | Status |
| --- | --- | --- | --- |
| Laptop | Zasilacz, aktualny Windows, wolne miejsce na dysku | Minimum kilka GB wolnego miejsca | Do zrobienia |
| Internet | Stabilne Wi-Fi, hotspot jako plan B | Wejdz na Google Cloud i GitHub | Do zrobienia |
| Chrome | Zainstalowana przegladarka Chrome | `chrome://version` | Gotowe - Chrome zainstalowany |
| Konto Google | Zalogowane konto prywatne lub wskazane przez organizatora | Wejdz na `https://myaccount.google.com/` | Gotowe w przegladarce wedlug uzytkownika |
| Google Cloud | Dostep do Console i zaakceptowane regulaminy | Wejdz na `https://console.cloud.google.com/` | Gotowe - zalogowane |
| Billing/kredyty | Aktywne OnRamp credits albo skonfigurowany billing, jesli wymagane | Google Cloud Console -> Billing -> Credits | Blokuje Cloud Run API - billing account `OPEN=False` |
| Git | Zainstalowany Git | `git --version` | Gotowe - Git 2.54.0 |
| Python | Python i pip | `python --version`; `python -m pip --version` | Gotowe lokalnie |
| Terminal | PowerShell lub Windows Terminal | Otworz terminal i uruchom komendy kontrolne | Gotowe - PowerShell dziala |
| Google AI Studio | Dostep do utworzenia Gemini API key | Wejdz na `https://aistudio.google.com/` | Czescowo - klucz jest, generowanie blokuje quota 0 |
| Repo warsztatowe | Mozliwosc sklonowania repo | `git clone https://github.com/speakleash/eskadra-bielik-misja1` | Gotowe - `C:\EB\m1` |
| ADK Web | Lokalny interfejs ADK startuje | `adk web --host 127.0.0.1 --port 8765` | Gotowe - HTTP 200 |
| Gemini API key | Klucz zapisany lokalnie | `C:\EB\m1\.env` | Czescowo - klucz rozpoznany, generowanie blokuje quota 0 |
| Bielik/Ollama endpoint | URL `OLLAMA_API_BASE` po deployu Bielika | Cloud Run URL | Czeka na zajecia/prowadzacego |
| `gcloud` lokalnie | Konto i projekt lokalny | `gcloud auth list`; `gcloud config get-value project` | Gotowe - konto i projekt ustawione |
| Cloud Run API | API wymagane do deployu Bielika/ADK | `gcloud services enable run.googleapis.com ...` | Zablokowane przez zamkniety billing |

## Instalacje Lokalne

### 1. Google Chrome

Wymagane albo mocno zalecane przez wydarzenia Eskadry Bielika.

Sprawdzenie:

```powershell
winget list Google.Chrome
```

Instalacja, jesli brakuje:

```powershell
winget install --id Google.Chrome --source winget
```

Po instalacji zaloguj sie w Chrome na konto Google, ktorego uzyjesz podczas zajec.

### 2. Git

Wymagany do pobrania materialow i pracy z repozytoriami.

Sprawdzenie:

```powershell
git --version
```

Instalacja, jesli brakuje:

```powershell
winget install --id Git.Git --source winget
```

Po instalacji otworz nowy terminal i sprawdz ponownie `git --version`.

### 3. Python

Python jest potrzebny do uruchamiania agentow, tworzenia virtualenv i instalowania zaleznosci przez `pip`.

Na tej maszynie Python zostal juz zainstalowany:

```powershell
python --version
python -m pip --version
```

Typowy workflow warsztatowy:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

Jesli PowerShell blokuje aktywacje virtualenv, uruchom jednorazowo:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

### 4. Visual Studio Code

Nie zawsze wymagany, ale bardzo wygodny do edycji `.env`, Pythona i notatek.

Sprawdzenie:

```powershell
code --version
```

Instalacja:

```powershell
winget install --id Microsoft.VisualStudioCode --source winget
```

Zalecane rozszerzenia:

- Python
- Jupyter
- GitHub Pull Requests and Issues
- Google Cloud Code, jesli planujesz pracowac z GCP lokalnie

### 5. Google Cloud CLI

Nie zawsze konieczne, bo oficjalny material Misji 1 prowadzi przez Cloud Shell. Warto miec lokalnie jako plan B.

Sprawdzenie:

```powershell
gcloud --version
```

Instalacja:

```powershell
winget install --id Google.CloudSDK --source winget
```

Pierwsze logowanie:

```powershell
gcloud init
gcloud auth login
gcloud auth application-default login
```

Uwaga: jesli prowadzacy uzywa Cloud Shell, lokalny `gcloud` moze nie byc potrzebny. Nie trac czasu na debugowanie lokalnego SDK na sali, jesli Cloud Shell dziala.

### 6. Docker Desktop

Opcjonalne. Przydatne, jesli chcesz uruchamiac kontenery lokalnie, ale oficjalny przeplyw Cloud Run moze budowac kontenery w chmurze.

Sprawdzenie:

```powershell
docker --version
```

Instalacja:

```powershell
winget install --id Docker.DockerDesktop --source winget
```

Po instalacji Docker Desktop zwykle wymaga restartu systemu albo wlaczonego WSL2.

### 7. LM Studio Albo Ollama

Opcjonalne. Przydatne, jesli chcesz testowac Bielika lokalnie poza warsztatem.

Bielik AI opisuje LM Studio jako prosty sposob lokalnego uruchomienia modelu. Lokalny model moze jednak wymagac duzo RAM/VRAM, szczegolnie dla wiekszych wersji i wyzszych kwantyzacji.

Instalacja LM Studio:

```powershell
winget install --id ElementLabs.LMStudio --source winget
```

Instalacja Ollama:

```powershell
winget install --id Ollama.Ollama --source winget
```

Na zajeciach priorytetem jest Google Cloud/Cloud Shell, nie lokalne katowanie laptopa modelem.

## Konta I Logowania

Przed wyjsciem na zajecia otworz Chrome i zaloguj sie do tych miejsc.

| Miejsce | Link | Po co | Status |
| --- | --- | --- | --- |
| Google Account | https://myaccount.google.com/ | Glowna tozsamosc do GCP, AI Studio i Cloud Shell | Do zrobienia |
| Google Cloud Console | https://console.cloud.google.com/ | Projekt, billing, Cloud Run, Cloud Shell, BigQuery | Do zrobienia |
| Google AI Studio | https://aistudio.google.com/ | Gemini API key dla ADK/hybrydowych agentow | Do zrobienia |
| GitHub | https://github.com/ | Klonowanie repo, ewentualne forki/notatki | Do zrobienia |
| Bielik chat | https://chat.bielik.ai/ | Szybki test modelu i punkt odniesienia | Do zrobienia |
| Luma/wydarzenie | Link z rejestracji | Potwierdzenie miejsca, adres, komunikaty organizatora | Do zrobienia |
| Discord/Slack organizatora | Link z maila, jesli jest | Komunikaty, pomoc techniczna, materialy po zajeciach | Do zrobienia |

## Google Cloud Readiness

### Minimalny stan

Przed zajeciami powinienes moc:

1. Wejsc do Google Cloud Console.
2. Otworzyc Cloud Shell.
3. Utworzyc albo wybrac projekt.
4. Sprawdzic Billing/Credits.
5. Skopiowac identyfikator projektu.

### Projekt GCP

Utworz projekt o czytelnej nazwie, np.:

```text
eskadra-bielika-<twoje-inicjaly>
```

Zapisz:

```text
Project name:
Project ID:
Billing account / credits:
Region preferowany: europe-west1
```

### Cloud Shell

W Google Cloud Console kliknij ikone Cloud Shell i sprawdz:

```bash
gcloud --version
git --version
python3 --version
```

Sklonowanie repo testowego:

```bash
git clone https://github.com/speakleash/eskadra-bielik-misja1
cd eskadra-bielik-misja1
cp .env.sample .env
```

Nie musisz deployowac wszystkiego przed zajeciami, jesli organizator ma wlasny kod wydarzenia albo kredyty OnRamp. Wazne, zeby Cloud Shell dzialal i zebys wiedzial, gdzie jest terminal.

### Billing I Kredyty

Materialy Misji 1 wspominaja o Cloud OnRamp credits albo skonfigurowanych platnosciach. Sprawdz to przed zajeciami:

1. Google Cloud Console.
2. Billing.
3. Credits.
4. Zapisz, czy masz aktywne kredyty.

Jesli nie masz kredytow i nie chcesz podpinac karty, zapytaj organizatora przed zajeciami, czy dostaniesz kod OnRamp albo gotowy projekt.

### Gemini API Key

Repo Misji 1 wymaga `GOOGLE_API_KEY` dla agentow ADK korzystajacych z Gemini.

Przygotowanie:

1. Wejdz na https://aistudio.google.com/.
2. Zaloguj sie tym samym kontem Google.
3. Utworz Gemini API key.
4. Nie wklejaj klucza do publicznego repo ani do chatu.
5. Na zajeciach wpisz go tylko do lokalnego/Cloud Shell pliku `.env`.

Przyklad wpisu w `.env`:

```bash
GOOGLE_API_KEY=twoj_klucz_api
```

## Pliki I Sekrety

Nie commituj i nie wysylaj:

- `.env`
- kluczy API
- tokenow Google
- URL publicznego Cloud Run, jesli prowadzacy traktuje go jako prywatny
- danych testowych organizatora, jesli nie zostaly oznaczone jako publiczne

Przygotuj lokalny katalog roboczy poza SparkleEngine, np.:

```powershell
mkdir C:\Users\stole\Documents\EskadraBielika
cd C:\Users\stole\Documents\EskadraBielika
```

Na tej maszynie praktyczna kopia warsztatowa jest w krotszej sciezce:

```powershell
cd C:\EB\m1
```

Ta sciezka jest preferowana dla lokalnych instalacji Pythona, bo zaleznosci `litellm` moga przekroczyc limit dlugosci sciezek Windows w dluzszych katalogach pod `Documents`.

## Test Przed Zajeciami

Uruchom w nowym PowerShellu:

```powershell
python --version
python -m pip --version
git --version
winget --version
```

Jesli zainstalowales Google Cloud CLI:

```powershell
gcloud --version
```

Jesli zainstalowales VS Code:

```powershell
code --version
```

Jesli zainstalowales Docker:

```powershell
docker --version
```

Test repo:

```powershell
cd C:\EB\m1\adk-agents
.\.venv\Scripts\Activate.ps1
python -m pip show google-adk litellm
adk --help
```

Test ladowania agentow:

```powershell
cd C:\EB\m1\adk-agents
.\.venv\Scripts\python.exe -c "import content_creator.agent as c; import culinary_guide_agent.agent as g; print(c.root_agent.name); print(g.root_agent.name)"
```

Oczekiwany wynik:

```text
content_creator_agent
culinary_guide_agent
```

Test lokalnego ADK Web bez wysylania promptow do modeli:

```powershell
cd C:\EB\m1\adk-agents
.\.venv\Scripts\adk.exe web --host 127.0.0.1 --port 8765
```

Nastepnie otworz w przegladarce:

```text
http://127.0.0.1:8765
```

Uwaga: lokalny ADK Web startuje, ale uruchomienie agentow moze wymagac:

- dzialajacego `OLLAMA_API_BASE` dla Bielika,
- dzialajacego limitu Gemini API,
- albo instrukcji/projektu/kredytow od prowadzacego.

Jesli trzeba odtworzyc virtualenv od zera:

```powershell
cd C:\EB\m1\adk-agents
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

Uwaga: oficjalne instrukcje repo sa pisane glownie pod Cloud Shell/Linux, wiec lokalny Windows moze roznic sie skladnia aktywacji venv i komend kopiowania plikow.

## Plan Dnia Zajec

Przed wyjsciem:

- Naladuj laptop.
- Wez zasilacz.
- Sprawdz, czy Chrome jest zalogowany do wlasciwego konta Google.
- Sprawdz, czy masz haslo/2FA do Google i GitHub.
- Sprawdz mail z rejestracja i adresem.
- Zapisz link do wydarzenia i kontakt do prowadzacego.
- Zrob restart laptopa po instalacjach, zeby PATH i uslugi systemowe byly odswiezone.

Na miejscu:

- Podlacz zasilanie od razu.
- Wejdz w Chrome na Google Cloud Console.
- Otworz Cloud Shell.
- Nie zaczynaj od lokalnego debugowania, jesli prowadzacy prowadzi przez Cloud Shell.
- Jesli cos nie dziala, najpierw sprawdz konto Google, projekt, billing/kredyty, region i aktywny projekt `gcloud`.

## Typowe Problemy I Szybkie Naprawy

| Problem | Szybka diagnoza | Naprawa |
| --- | --- | --- |
| `python` otwiera Microsoft Store | `where.exe python` pokazuje WindowsApps jako pierwsze | Otworz nowy terminal; sprawdz PATH; uzyj pelnej sciezki do CPython |
| `git` nie dziala po instalacji | Terminal byl otwarty przed instalacja | Zamknij i otworz nowy terminal |
| `Activate.ps1` jest blokowany | PowerShell execution policy | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` |
| Cloud Shell nie startuje | Konto/region/przegladarka | Odwiezenie strony, inne konto Chrome, incognito, sprawdzenie 2FA |
| Brak billing/kredytow | Cloud Run/GPU moze sie nie wdrozyc | Zapytaj prowadzacego o OnRamp credits albo gotowy projekt |
| `gcloud services enable` zwraca `Billing account ... is not open` | Projekt nie ma aktywnego billing/free trial albo billing account jest zamkniety | Otworz Billing -> linked account dla projektu albo poczekaj na OnRamp/projekt prowadzacego |
| `GOOGLE_API_KEY` nie dziala | Zly projekt albo ograniczenia klucza | Utworz nowy klucz w AI Studio na wlasciwym koncie |
| Gemini API zwraca `429 RESOURCE_EXHAUSTED` | Klucz jest rozpoznany, ale projekt/model ma quota 0 albo wyczerpany limit | Nie kupuj prepaid bez instrukcji; zapytaj prowadzacego o projekt/kredyty albo uzyj limitu warsztatowego |
| Agenci startuja, ale nie odpowiadaja | Brak `OLLAMA_API_BASE`, brak quota Gemini albo brak deployu Bielika | Najpierw skonfiguruj Cloud Run/Bielika wedlug prowadzacego |
| Deploy Cloud Run trwa dlugo | Budowanie obrazu i pobieranie modelu | Czekaj; sprawdz Cloud Build logs i Cloud Run logs |
| Publiczny URL Cloud Run | `--allow-unauthenticated` wystawia usluge | Uzywaj tylko do warsztatu; po zajeciach usun usluge |

## Cleanup Po Zajeciach

Zeby uniknac kosztow i wyciekow:

1. Usun albo zatrzymaj uslugi Cloud Run utworzone na warsztacie.
2. Sprawdz Cloud Billing.
3. Usun niepotrzebne API keys z Google AI Studio.
4. Nie publikuj `.env`.
5. Zachowaj notatki i repo, ale bez sekretow.

Komendy pomocnicze w Cloud Shell:

```bash
gcloud run services list
gcloud run services delete SERVICE_NAME --region europe-west1
```

## Wymaga Twojej Interakcji

Tych rzeczy agent nie powinien robic sam, bo wymagaja Twojego konta, 2FA, sekretow albo decyzji kosztowo-systemowej:

| Zadanie | Dlaczego wymaga Ciebie | Komenda albo miejsce |
| --- | --- | --- |
| Billing/free trial dla projektu GCP | API Cloud Run/Cloud Build nie da sie wlaczyc przy billing account `OPEN=False` | Google Cloud Console -> Billing -> Linked billing account |
| Potwierdzenie projektu warsztatowego | Obecnie ustawiony jest `project-4efc201e-a329-4966-822`, ale prowadzacy moze wymagac innego projektu | `gcloud config set project PROJECT_ID` |
| Gemini API billing/quota | Obecny klucz jest rozpoznany, ale generowanie ma quota 0; zakup prepaid to decyzja kosztowa | Nie kupowac bez polecenia prowadzacego |
| Stary ujawniony API key | Klucz raz wklejony do chatu nalezy traktowac jako ujawniony | Usun stary klucz w Google AI Studio |
| Kod `BIELIK_EVENT_ID` / OnRamp | Zalezy od organizatora i wydarzenia | Mail/Luma/prowadzacy |
| `OLLAMA_API_BASE` | Powstanie dopiero po deployu Bielika do Cloud Run albo po podaniu gotowego endpointu | Wpis do `C:\EB\m1\.env` |
| Docker Desktop | Opcjonalny, ciezki, moze wymagac restartu/WSL2 | Zainstalowac tylko jesli chcesz |
| LM Studio/Ollama lokalnie | Opcjonalne, moze pobierac duze modele | Zainstalowac tylko jesli chcesz testowac Bielika lokalnie |

## Decyzja Gotowosci

Maszyna jest gotowa na zajecia, gdy:

- Chrome, Git i Python dzialaja w nowym terminalu.
- Jestes zalogowany do Google, Google Cloud Console, Google AI Studio i GitHub.
- Cloud Shell sie otwiera.
- Wiesz, czy masz billing/kredyty OnRamp.
- Masz przygotowany katalog roboczy poza repo SparkleEngine.
- Masz zasilacz, 2FA i link do wydarzenia.

Aktualna ocena 2026-06-12:

- Maszyna lokalna jest przygotowana technicznie.
- Repo warsztatowe i ADK dzialaja lokalnie na poziomie instalacji, importow i UI.
- `gcloud` jest zalogowany i ma ustawiony projekt `project-4efc201e-a329-4966-822`.
- Pelne uruchomienie agentow czeka na dane/projekt od prowadzacego: `BIELIK_EVENT_ID`, OnRamp/kredyty albo decyzje billingowa, oraz `OLLAMA_API_BASE`.
- Deploy Cloud Run jest obecnie zablokowany, bo billing account projektu jest zamkniety (`OPEN=False`).
- Nie kupuj Gemini prepaid ani nie wlaczaj platnego API bez jasnej instrukcji prowadzacego.

Jesli te punkty sa zielone, jestes w dobrej pozycji: nawet gdy lokalne narzedzie odmowi wspolpracy, Cloud Shell powinien pozwolic zrobic zasadnicza czesc warsztatu.
