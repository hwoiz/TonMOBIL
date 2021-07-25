// ########################################### loop() #######################################################
void loop() {
  do {
    checkStandbyAtMillis();
    mp3.loop();

    // Modifier : WIP!
    if (activeModifier != NULL) {
      activeModifier->loop();
    }

#ifdef LED_SR
    // LED Strip
    //Prüfung der einmaligen Animationen 
    // Liedänderung erkennen und Animation aktivieren
    currentDetectedTrack = currentTrack;
    if (currentDetectedTrack != lastDetectedTrack){
      strip.clear();
      if (currentTrack > lastDetectedTrack) { //nächstes Lied
        lsrAnimationTrackMode = 1;
        lsrColors = lsrColorUp;
      }
      if (currentTrack < lastDetectedTrack) { // Lied zurück
        lsrAnimationTrackMode = 2;
        lsrColors = lsrColorDown;
      }
      lsrAnimationMode = 1;
      animationCountdown = strip.numPixels();
      lsrLoopCountWait = 5; // Geschwindigkeit der Animation, desto größer desto langsamer
      y = 0;
    }

    // Lautstärkenanpassung erkennen und Animation aktivieren
    currentDetectedVolume = volume;
    if (currentDetectedVolume != lastDetectedVolume) {
      lsrAnimationMode = 2;
      animationCountdown = strip.numPixels();
      lsrLoopCountWait = 6;
      y = 0;
    }

    // Dauerhafte Loop Animationen
    // Loop Animation: Default Mode
    if (lsrAnimationMode == 0 && loopCountdown == 0 && isPlaying() == false && knownCard == false) {
      lsrLoopCountWait = 1; // Geschwindigkeit der Animation, desto größer desto langsamer

      // Farbe & Animation definieren: Alle LEDs leuchten alle abwechselnd  im hue Spektrum
      y++;
      if (y >= (strip.numPixels() * 8) ) {
        y = 0;
      }
      strip.fill(strip.ColorHSV((y * 65536 / strip.numPixels() / 8) , 255, 30), 0, 0);
      strip.show();
      loopCountdown = lsrLoopCountWait;
    }

    // Loop Animation: Musik spielt
    if (lsrAnimationMode == 0 && loopCountdown == 0 && isPlaying() == true && knownCard == true) {
      lsrLoopCountWait = 5; // Geschwindigkeit der Animation, desto größer desto langsamer
      // Fabre definieren: hue Spektrum (Rainbow)
      do {
        for (i = 0; i < strip.numPixels(); i++) {
          lsrColors = strip.ColorHSV(i * 65536 / strip.numPixels(), 255, 30);
          strip.setPixelColor(i, lsrColors);
          lsrColorR[i] = (lsrColors >> 16 & 0xFF);
          lsrColorG[i] = (lsrColors >> 8 & 0xFF);
          lsrColorB[i] = (lsrColors & 0xFF);
        }
        x++;
      } while (x < strip.numPixels());
      // Animation definieren: Rotation im Uhrzeigersinn
      y++;
      x = 0;
      if (y >= strip.numPixels())
      {
        y = 0;
      }
      do
      {
        for (i = 0; i < strip.numPixels(); i++)
        {
          strip.setPixelColor((i + y) % strip.numPixels(), lsrColorR[i], lsrColorG[i], lsrColorB[i]);
        }
        x++;
      } while (x < strip.numPixels());

      strip.show();
      loopCountdown = lsrLoopCountWait;
    }

    // Loop Animation: Musik pausiert
    if (lsrAnimationMode == 0 && loopCountdown == 0 && isPlaying() == false && knownCard == true) {
      lsrLoopCountWait = 7; // Geschwindigkeit der Animation, desto größer desto langsamer
      // Fabre definieren: hue Spektrum (Rainbow)
      strip.clear();   // Danach nur ein Punkt
      x = 0;
      do {
        for (i = 0; i < strip.numPixels(); i++) {
          lsrColors = strip.ColorHSV(i * 65536 / strip.numPixels(), 255, 30);
          lsrColorR[i] = (lsrColors >> 16 & 0xFF);
          lsrColorG[i] = (lsrColors >> 8 & 0xFF);
          lsrColorB[i] = (lsrColors & 0xFF);
        }
        x++;
      } while (x < strip.numPixels());
      // Farbe definieren: Füllend ansteigend
      y++;
      if (y >= strip.numPixels()) {
        y = 0;
        z++;
        strip.clear();
      }
      if (z >= strip.numPixels()) {
        z = 0;
      }
      x = 0;
      do {
        for (i = 0; i < y + 1 ; i++) {
          strip.setPixelColor( y , lsrColorR[y], lsrColorG[y], lsrColorB[y]);
        }
        x++;
      } while (x < y + 1);
      strip.show();
      loopCountdown = lsrLoopCountWait;
    }

    // Einmalige Animationen bei einem Ereignis 
    // Einmalige Animation: Liedänderung
    if (lsrAnimationMode == 1 && loopCountdown == 0) {
      // Fabre definieren: oben definiert
      x = 0;
      do {
        for (i = 0; i < strip.numPixels(); i++) {
          lsrColorR[i] = (lsrColors >> 16 & 0xFF);
          lsrColorG[i] = (lsrColors >> 8 & 0xFF);
          lsrColorB[i] = (lsrColors & 0xFF);
        }
        x++;
      } while (x < strip.numPixels());
      // Animation definieren: oben definiert
      if (y >= strip.numPixels()) {
        strip.clear();
        y = 0;
      }
      if (lsrAnimationTrackMode == 1) {
        z = y ;
      }
      if (lsrAnimationTrackMode == 2) {
        z = strip.numPixels() - y ;
      }
      x = 0;
      do {
        for (i = 0; i < y + 1 ; i++) {
          strip.setPixelColor( z , lsrColorR[y], lsrColorG[y], lsrColorB[y]);
        }
        x++;
      } while (x < y + 1);
      y++;
      strip.show();
      if (animationCountdown != 0) {
        animationCountdown--;
      }
      if (animationCountdown == 0) {
        lsrAnimationMode = 0;
      }
      loopCountdown = lsrLoopCountWait ;
    }
    // Einmalige Animation: Prozentuale Lautstärkenanpassung
    if (lsrAnimationMode == 2 && loopCountdown == 0) {
      if (animationCountdown != 0) {
        animationCountdown--;
      }
      if (currentDetectedVolume != lastDetectedVolume) {
        lsrLoopCountWait = 5;
      }
      // Lautstärkenanzeige angepasst an die Anzahl der LEDs
      volumeScope = (mySettings.maxVolume - mySettings.minVolume);
      volumeScopeAmount = (volume - mySettings.minVolume) * (LED_COUNT - 1) / volumeScope; 
      // Fabre definieren: von grün zu rot
      x = 0;
      do {
        for (i = 0; i < strip.numPixels(); i++) {
          lsrHueCalc = 21000 / (strip.numPixels() - 1) / (strip.numPixels() - 1);
          lsrColors = strip.ColorHSV(((strip.numPixels() - 1) - i) * (strip.numPixels() - 1) * lsrHueCalc, 255, 30);
          strip.setPixelColor(i, lsrColors);
          lsrColorR[i] = (lsrColors >> 16 & 0xFF);
          lsrColorG[i] = (lsrColors >> 8 & 0xFF);
          lsrColorB[i] = (lsrColors & 0xFF);
        }
        x++;
      } while (x < strip.numPixels());
      // Animation definieren: Prozentuale Lautstärkenanpassung
      strip.clear();
      x = 0;
      do {
        for (i = 0; i < volumeScopeAmount + 1; i++) {
          strip.setPixelColor(i, lsrColorR[i], lsrColorG[i], lsrColorB[i]);
        }
        x++;
      } while (x < (volumeScopeAmount + 1));
      strip.show();
      if (animationCountdown == 0) {
        //delay(20);
        lsrAnimationMode = 0;
      }
      loopCountdown = lsrLoopCountWait;
    }
    // Countdown Zähler über den loop als ersatz zur delay Funktion
    if (loopCountdown != 0 ) {
      loopCountdown--;
    }
    // Dadurch wird die Änderung der Lautstärke bzw. Track nur ein mal registiert
    lastDetectedVolume = currentDetectedVolume;
    lastDetectedTrack = currentDetectedTrack;
#endif

    // Buttons werden nun über JS_Button gehandelt, dadurch kann jede Taste doppelt belegt werden
    readButtons();
    // admin menu
    if ((pauseButton.pressedFor(LONG_PRESS) || upButton.pressedFor(LONG_PRESS) || downButton.pressedFor(LONG_PRESS)) && pauseButton.isPressed() && upButton.isPressed() && downButton.isPressed()) {
      mp3.pause();
      do {
        readButtons();
      } while (pauseButton.isPressed() || upButton.isPressed() || downButton.isPressed());
      readButtons();
      adminMenu();
      break;
    }
    if (pauseButton.wasReleased()) {
      if (activeModifier != NULL)
        if (activeModifier->handlePause() == true)
          return;
      if (ignorePauseButton == false)
        if (isPlaying()) {
          mp3.pause();
          setstandbyTimer();
        }
        else if (knownCard) {
          mp3.start();
          disablestandbyTimer();
        }
      ignorePauseButton = false;
    } else if (pauseButton.pressedFor(LONG_PRESS) &&
               ignorePauseButton == false) {
      if (activeModifier != NULL)
        if (activeModifier->handlePause() == true)
          return;
      if (isPlaying()) {
        uint8_t advertTrack;
        if (myFolder->mode == 3 || myFolder->mode == 9) {
          advertTrack = (queue[currentTrack - 1]);
        }
        else {
          advertTrack = currentTrack;
        }
        // Spezialmodus Von-Bis für Album und Party gibt die Dateinummer relativ zur Startposition wieder
        if (myFolder->mode == 8 || myFolder->mode == 9) {
          advertTrack = advertTrack - myFolder->special + 1;
        }
        mp3.playAdvertisement(advertTrack);
      }
      else {
        playShortCut(0);
      }
      ignorePauseButton = true;
    }

    if (upButton.pressedFor(LONG_PRESS)) {
#ifndef FIVEBUTTONS
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeUpButton();
        }
        else {
          nextButton();
        }
      }
      else {
        playShortCut(1);
      }
      ignoreUpButton = true;
#endif
    } else if (upButton.wasReleased()) {
      if (!ignoreUpButton)
        if (!mySettings.invertVolumeButtons) {
          nextButton();
        }
        else {
          volumeUpButton();
        }
      ignoreUpButton = false;
    }
    if (downButton.pressedFor(LONG_PRESS)) {
#ifndef FIVEBUTTONS
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeDownButton();
        }
        else {
          previousButton();
        }
      }
      else {
        playShortCut(2);
      }
      ignoreDownButton = true;
#endif
    } else if (downButton.wasReleased()) {
      if (!ignoreDownButton) {
        if (!mySettings.invertVolumeButtons) {
          previousButton();
        }
        else {
          volumeDownButton();
        }
      }
      ignoreDownButton = false;
    }
#ifdef FIVEBUTTONS
    if (buttonFour.wasReleased()) {
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeUpButton();
        }
        else {
          nextButton();
        }
      }
      else {
        playShortCut(1);
      }
    }
    if (buttonFive.wasReleased()) {
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeDownButton();
        }
        else {
          previousButton();
        }
      }
      else {
        playShortCut(2);
      }
    }
#endif
    // Ende der Buttons
  } while (!mfrc522.PICC_IsNewCardPresent());

  // RFID Karte wurde aufgelegt
  if (!mfrc522.PICC_ReadCardSerial())
    return;
  // Bekannte Karte - Spielt Album ab
  if (readCard(&myCard) == true) {
    if (myCard.cookie == cardCookie && myCard.nfcFolderSettings.folder != 0 && myCard.nfcFolderSettings.mode != 0) {
      playFolder();
    }
    // Neue Karte konfigurieren - Programmierung wird aufgerufen
    else if (myCard.cookie != cardCookie) {
      knownCard = false;
      mp3.playMp3FolderTrack(300);
      waitForTrackToFinish();
      setupCard();
    }
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
} // ##################### Ende von loop() ################################
