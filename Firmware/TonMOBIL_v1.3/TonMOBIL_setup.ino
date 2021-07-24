// ########################################### setup() #######################################################
void setup() {

  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle

  // Wert für randomSeed() erzeugen durch das mehrfache Sammeln von rauschenden LSBs eines offenen Analogeingangs
  uint32_t ADC_LSB;
  uint32_t ADCSeed;
  for (uint8_t i = 0; i < 128; i++) {
    ADC_LSB = analogRead(openAnalogPin) & 0x1;
    ADCSeed ^= ADC_LSB << (i % 32);
  }
  randomSeed(ADCSeed); // Zufallsgenerator initialisieren

  // Dieser Hinweis darf nicht entfernt werden
  Serial.println(F("  _______          __  __  ____  ____ _____ _      "));
  Serial.println(F(" |__   __|        |  \/  |/ __ \|  _ \_   _| |     "));
  Serial.println(F("    | | ___  _ __ | \  / | |  | | |_) || | | |     "));
  Serial.println(F("    | |/ _ \| '_ \| |\/| | |  | |  _ < | | | |     "));
  Serial.println(F("    | | (_) | | | | |  | | |__| | |_) || |_| |____ "));
  Serial.println(F("    |_|\___/|_| |_|_|  |_|\____/|____/_____|______|"));
  Serial.println(F(" "));
  Serial.print(F("TonMOBIL Version ")); Serial.println(version);
  Serial.println(F("Harald Woizischke"));
  Serial.println(F(" "));
  Serial.println(F("Weiterentwicklung von TonUINO Version 2.1, Torsten Voss https://tonuino.de"));
  Serial.println(F("Lizenziert unter GNU/GPL"));
  Serial.println(F("Projektrepository https://github.com/hwoiz/TonMOBIL"));
  Serial.print(F("Flashversion ")); Serial.print(__DATE__); Serial.print(F("  ")); Serial.println(__TIME__);


  // Busy Pin
  pinMode(busyPin, INPUT);

  // load Settings from EEPROM
  loadSettingsFromFlash();

  // activate standby timer
  setstandbyTimer();

#ifdef LED_SR
  strip.begin();
  strip.setBrightness(30);
  strip.fill(30, 0, LED_COUNT);
  strip.show();

  loopCountdown = 0;
  animationCountdown = 1;
  lastDetectedTrack = 0;

#endif

  // DFPlayer Mini initialisieren
  mp3.begin();
  Serial.println(F("Init MP3-Player"));
  // Drei Sekunden warten bis der DFPlayer Mini initialisiert ist
  delay(3000);
  volume = mySettings.initVolume;
  mp3.setVolume(volume);
  mp3.setEq(mySettings.eq - 1);

  // NFC Leser initialisieren
  Serial.println(F("Init NFC-Reader"));
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  mfrc522
  .PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("Init Button"));
  pinMode(buttonPause, INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
#ifdef FIVEBUTTONS
  pinMode(buttonFourPin, INPUT_PULLUP);
  pinMode(buttonFivePin, INPUT_PULLUP);
#endif
  Serial.println(F("Init ShutdownPin"));
  pinMode(shutdownPin, OUTPUT);
#if defined POLOLUSWITCH
  digitalWrite(shutdownPin, LOW);
#else
  digitalWrite(shutdownPin, HIGH);
#endif


  // RESET --- ALLE DREI KNÖPFE BEIM STARTEN GEDRÜCKT HALTEN -> alle EINSTELLUNGEN werden gelöscht
  if (digitalRead(buttonPause) == LOW && digitalRead(buttonUp) == LOW &&
      digitalRead(buttonDown) == LOW) {
    Serial.println(F("Reset -> EEPROM wird gelöscht"));
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.update(i, 0);
    }
    loadSettingsFromFlash();
  }

  // welcome sound
  //  mp3.playMp3FolderTrack(801);

  //Start "Shortcut at Startup"
  playShortCut(3);

  Serial.println(F("TonUINO ready"));
} // ######################### Ende von setup() ######################################

// ########################### Funktionen definieren  ################################
void readButtons() {
  pauseButton.read();
  upButton.read();
  downButton.read();
#ifdef FIVEBUTTONS
  buttonFour.read();
  buttonFive.read();
#endif
}

void volumeUpButton() {
  if (activeModifier != NULL)
    if (activeModifier->handleVolumeUp() == true)
      return;

  Serial.println(F("=== volumeUp()"));
  if (volume < mySettings.maxVolume) {
    mp3.increaseVolume();
    volume++;
  }
  Serial.println(volume);
}

void volumeDownButton() {
  if (activeModifier != NULL)
    if (activeModifier->handleVolumeDown() == true)
      return;

  Serial.println(F("=== volumeDown()"));
  if (volume > mySettings.minVolume) {
    mp3.decreaseVolume();
    volume--;
  }
  Serial.println(volume);
}

void nextButton() {
  if (activeModifier != NULL)
    if (activeModifier->handleNextButton() == true)
      return;

  nextTrack(random(65536));
  delay(1000);
}

void previousButton() {
  if (activeModifier != NULL)
    if (activeModifier->handlePreviousButton() == true)
      return;

  previousTrack();
  delay(1000);
}

void playFolder() {
  disablestandbyTimer();
  knownCard = true;
  ansage = true; // Verhindert nextTrack()
  Serial.print(F("== playFolder("));
  Serial.print("Album "); Serial.print(myFolder->folder); Serial.println(")");
  Serial.print("== Album-Ansage 000 spielen in Ordner "); Serial.println(myFolder->folder);
  mp3.playFolderTrack(myFolder->folder, 0);
  delay(3000); // Warten - Der Track 000 muss innerhalb dieser Zeit starten
  waitForTrackToFinish();  //Warten bis Ansage der Ordnernummer beendet
  ansage = false; // Erlaubt nextTrack
  Serial.println(F("== Anzahl der Dateien im Ordner werden ermitteln"));
  numTracksInFolder = mp3.getFolderTrackCount(myFolder->folder) - 1; // Die Ansage wird abgezogen
  Serial.print(numTracksInFolder); Serial.print(F(" Dateien in Ordner ")); Serial.println(myFolder->folder);
  _lastTrackFinished = 0;
  firstTrack = 1;

  // Hörspielmodus: eine zufällige Datei aus dem Ordner
  if (myFolder->mode == 1) {
    Serial.println(F("Hörspielmodus -> zufälligen Track wiedergeben"));
    currentTrack = random(1, numTracksInFolder + 1);
    Serial.println(currentTrack);
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Album Modus: kompletten Ordner spielen
  if (myFolder->mode == 2) {
    Serial.println(F("Album Modus -> kompletten Ordner wiedergeben"));
    currentTrack = 1;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Party Modus: Ordner in zufälliger Reihenfolge
  if (myFolder->mode == 3) {
    Serial.println(
      F("Party Modus -> Ordner in zufälliger Reihenfolge wiedergeben"));
    shuffleQueue();
    currentTrack = 1;
    Serial.print(F("Spiele Titel ")); Serial.println(queue[currentTrack - 1]);
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
  // Einzel Modus: eine Datei aus dem Ordner abspielen
  if (myFolder->mode == 4) {
    Serial.println(
      F("Einzel Modus -> eine Datei aus dem Odrdner abspielen"));
    currentTrack = myFolder->special;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken
  if (myFolder->mode == 5) {
    Serial.println(F("Hörbuch Modus -> kompletten Ordner spielen und "
                     "Fortschritt merken"));
    currentTrack = EEPROM.read(myFolder->folder);
    if (currentTrack == 0 || currentTrack > numTracksInFolder) {
      currentTrack = 1;
    }
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Spezialmodus Von-Bin: Hörspiel: eine zufällige Datei aus dem Ordner
  if (myFolder->mode == 7) {
    Serial.println(F("Spezialmodus Von-Bin: Hörspiel -> zufälligen Track wiedergeben"));
    Serial.print(myFolder->special);
    Serial.print(F(" bis "));
    Serial.println(myFolder->special2);
    numTracksInFolder = myFolder->special2;
    currentTrack = random(myFolder->special, numTracksInFolder + 1);
    Serial.println(currentTrack);
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }

  // Spezialmodus Von-Bis: Album: alle Dateien zwischen Start und Ende spielen
  if (myFolder->mode == 8) {
    Serial.println(F("Spezialmodus Von-Bis: Album: alle Dateien zwischen Start- und Enddatei spielen"));
    Serial.print(myFolder->special);
    Serial.print(F(" bis "));
    Serial.println(myFolder->special2);
    numTracksInFolder = myFolder->special2;
    currentTrack = myFolder->special;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }

  // Spezialmodus Von-Bis: Party Ordner in zufälliger Reihenfolge
  if (myFolder->mode == 9) {
    Serial.println(
      F("Spezialmodus Von-Bis: Party -> Ordner in zufälliger Reihenfolge wiedergeben"));
    firstTrack = myFolder->special;
    numTracksInFolder = myFolder->special2;
    shuffleQueue();
    currentTrack = 1;
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
}

void playShortCut(uint8_t shortCut) {
  Serial.println(F("=== playShortCut()"));
  Serial.println(shortCut);
  if (mySettings.shortCuts[shortCut].folder != 0) {
    myFolder = &mySettings.shortCuts[shortCut];
    playFolder();
    disablestandbyTimer();
    delay(1000);
  }
  else
    Serial.println(F("Shortcut not configured!"));
}
