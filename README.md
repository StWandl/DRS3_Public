Ordnerstruktur bzw. Inhalte:

* Rpi_Testsystem: Software zum Zustandsüberwachen der einzelnen Knoten, für Testaufbau
* SyncAnalyzer:   Matlab-Skript um Logic-Analyzer Daten zu analysieren und darzustellen + Messdaten bzw. Ergebnisse
* SyncModuleTest: Modultest Software, um den Synchronisationsablauf einzelner Knoten in Software zu testen

----------------------------------------------------------

**Bezüglich SyncModuleTest**

Benötigt boost Bibliothek ( https://www.boost.org/users/download/ ), entwickelt mit boost_1_67_0-msvc-14.1-64

Testablauf:
1) Start von diesem Testprogramm (wartet 20sek auf erste Nachricht von Implementierung)
2) Start von Implementierung

Aufruf: ./SyncModuleTest(.exe) [1] [2] [3]

Kommandozeilen-Parameter:
* [1]: Test-Szenario, mögliche Werte: 1, 2
* [2]: Priorität von Testobjekt, mögliche Werte: > 1
* [3]: -local, optional. Gibt an ob Testprogramm und Testobjekt auf der gleichen Maschine laufen

Ist -local gesetzt, dann sendet das Testprogramm Nachrichten auf den Port 12346 und empfängt auf dem Port 12345.

Ist -local nicht gesetzt, dann sendet und empfängt das Testprogramm alle Nachrichten über Port 12345.

Beispielaufrufe:
* ./SynchModuleTest.exe 1 100 -local
* ./SynchModuleTest.exe 2 100
* ./SynchModuleTest.exe 2 50 -local
* ...

Für genauere Beschreibungen der beiden Testszenarien siehe Source-Code oder Test-Szenario1.txt und Test-Szenario2.txt.