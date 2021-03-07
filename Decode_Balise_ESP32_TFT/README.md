<img width = "400" src="TTGOCapture1.png" />

* Decode la trame beacon d'un balise, affiche le résultat sur le TFT d'une carte ESP32 TTGO-T-Display et sur le port série 
* Compilation avec une carte ESP32 Dev Module

### Préparation de la librairie

  1. Charger TFT_eSPI-master.zip depuis https://github.com/Bodmer/TFT_eSPI en cliquant sur **Code** en vert en haut à droite.
  2. Copier le .ZIP dans la libraries avec Croquis->Inclure une bibliothèque->Ajouter le bibliothèque .ZIP ...
  3. Dans le fichier libraries\TFT_eSPI\User_Setup_Select.h; décommenter la ligne #include <User_Setups/Setup25_TTGO_T_Display.h>
  4. Dans le fichier User_Setup.h; commenter la ligne #define ILI9341_DRIVER

#### Notes: 
 *  Le driver de l'afficheur TFT 1.14" est déjà déclaré dans le code et non pas dans le fichier libraries\TFT_eSPI\User_Setup.h
  ```
  //Driver carte TTGO T-Display
  #define ST7789_DRIVER
  ```
  * Le chemin du répertoire **libraries** est affiché dans Fichier->Préférences->Paramètres->Emplacement du carnet de croquis

## Mode d'emploi

Une pression sur le bouton en haut à droite augmente la temporisation d'affichage de +100 ms.

Une pression sur le bouton en bas à droite diminue la temporisation d'affichage de -100 ms.

La valeur de temporisation est affichée dans le coin en haut à gauche.
