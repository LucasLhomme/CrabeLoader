# Journal des versions

Format : [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/), versions [SemVer](https://semver.org/lang/fr/).
Commits : `type(zone): résumé` (types `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `chore`), avec un corps qui
dit pourquoi et comment c'est vérifié. Publier une version : mettre à jour `VERSION` et ce journal ; GitHub
Actions crée alors l'étiquette et la release.

## [Non publié]
### Corrigé
- Nettoyage Blender des fichiers `.dae` : Blender 5 n'a plus d'import COLLADA. Le `.dae` est lu par Creator
  (pièces fusionnées, textures en un atlas) puis confié à Blender en `.glb` avec un matériau PBR neutre, ce qui
  garde les couleurs à l'identique (vérifié sur Ganondorf Smash : texture moyenne 54,35 avant et après).
- Rapport du nettoyage : la sortie de Blender est lue en UTF-8 ; sous Windows, les accents cassés empêchaient de
  filtrer les avertissements d'orientation sans objet (« Modèle debout »).
- Aperçu « in game pose » : le squelette de base (fantôme et points) suit la pose de jeu au lieu de rester en
  T-pose, et l'aperçu se met à jour en direct avec les curseurs du squelette.
### Amélioré
- Auto-fit du squelette calibré sur les figures du jeu (`tools/fit_calibration.py`) : 70 figures humanoïdes remises
  en T-pose et mesurées comme un modèle importé ; chaque articulation est prédite par un petit modèle linéaire.
  Erreur (validation croisée, % de la hauteur) : largeur d'épaules 4,3 → 1,7 (les manches bouffantes et les
  cheveux ne trompent plus la détection : Zelda 0,37 → 0,13), mains 6,2 → 2,9 (poignet trouvé sur l'avant-bras
  corrigé), profondeur genoux 3,0 → 2,0, cou 4,6 → 4,3. Hanches et hauteur des bras volontairement non corrigées
  (les proportions des figures, grosse tête et jambes courtes, fausseraient un modèle réaliste). Test aller-retour
  sur 69 figures : écart médian 4,4 % → 4,1 %.
### Outils
- `tools/roundtrip.py` (développeur) : test « aller-retour ». Une figure officielle est remise en T-pose avec son
  propre squelette, importée dans Creator, riggée sur elle-même et comparée à l'originale (forme en pose de jeu,
  poids, squelette auto). Options `--true-skeleton` et `--official-weights` pour isoler chaque source d'erreur.
### Corrigé
- Menton / cou étirés en pose de jeu quand le torse ou le cou du modèle n'a pas la longueur de celui de la base
  (Sora sur Gamora : 1,8 cm puis 1,1 cm d'écart) : le cou part maintenant du bout du torse et la tête du bout du
  cou au lieu de sauter sur les articulations de la base. La zone rigide de la tête ne prend plus le col, la
  capuche ni le haut des épaules à côté du cou (seulement ce qui est au-dessus du cou, et tout ce qui dépasse
  l'articulation de la tête : cheveux, chapeau).
- Les points du squelette restent visibles avec « Preview in game pose » (et en « In-game pose » une fois
  riggé) : ils suivent le modèle dans la pose du jeu.
### Ajouté
- Relief (case « Relief », onglet Rig & Export, activée par défaut) : une normal map est tirée de la texture du
  modèle (détails fins, sombre = creusé, bords de couleur = rainure légère) au format des cartes du jeu (R = 255,
  G = Y, A = X). Les 37 figures officielles doivent leur aspect « figurine sculptée » à ces cartes (coutures, plis,
  boutons) ; le Creator n'envoyait qu'une carte plate. Inclinaison calibrée sur 40 cartes du jeu (médiane ~4°,
  coutures ~31° contre ~35°).
### Corrigé
- Sens des tangentes (signe de la bitangente) inversé par rapport au jeu sur toutes les figures : vérifié sur les
  37 figures officielles (98 à 99 % des sommets maintenant identiques). Sans effet avec une carte plate, mais
  indispensable au relief (les creux seraient sortis en bosses).
- Icônes du personnage refaites au format réel du jeu : les fichiers produits jusqu'ici suivaient le gabarit de
  `make_icon.py` (CrabeLoader), qui ne correspond à aucun fichier du jeu, et rien ne pointait vers eux. Le Creator
  écrit maintenant, à partir de captures du modèle :
  - le portrait HUD 128 px `ui/icons/hud_playericons_<acteur>.mtb` + texture, référencé par `mod.json`
    (`"icon": "HUD_PlayerIcons_<Acteur>"`, que CrabeLoader recopie dans la ligne du personnage) ;
  - la figure du Virtual Reader `ui/virtualreadercharacters/<acteur>.mtb` (512 px), `_sm` (256 px) et la
    silhouette lumineuse `_s` (512 px).
  Les bundles sont identiques octet pour octet à ceux du jeu (vérifié sur Anakin) ; textures DXT5 à un niveau
  comme les originales. Aperçu des trois images avec le bouton « Preview ».
- Aperçu « Preview in game pose » (onglet Squelette) : il utilise maintenant les poids de secours du vrai rig
  (zones du corps, tête rigide) au lieu de poids approximatifs. Le menton ne s'étire plus vers le col et les
  épaules ne remontent plus vers les oreilles : l'aperçu ressemble au rig construit.
### Ajouté
- Os de torsion du jeu (case « Game twist bones », activée par défaut) : les 37 figures officielles partagent le
  haut du bras, l'avant-bras et la cuisse entre l'os principal et des os d'aide pilotés par le moteur
  (`Shoulder_Spin`, `Shoulder_OL`, `Forearm_OL`, `Thigh_OL`, ≈ 7 % du poids du corps). Le Creator ne pesait que
  l'os principal : l'épaule et la hanche tournaient d'un bloc avec le membre. Le partage mesuré sur ces figures
  (profil médian le long du membre, `tools/twist_profiles.py`) est maintenant copié sur le modèle. Au repos rien
  ne bouge ; l'effet n'est visible qu'en jeu.
- `Sync_Claude.bat` (+ `tools/pack_assets.py`) : regroupe `src/assets` (≈ 900 fichiers extraits du jeu, non
  versionnés) en une archive `build/claude_sync/assets.zip`, transférable en une fois vers une session cloud.
### Corrigé
- Import d'un .zip à plusieurs modèles : le Creator prend le personnage principal (nom de l'archive, pas de
  variante « ghost/phantom », pas d'accessoire, le plus haut dans les dossiers) au lieu du plus gros fichier ; si
  l'archive en contient plusieurs, une fenêtre propose la liste (son choix en premier). Le nettoyage Blender
  utilise le même fichier.
- Orientation automatique : les modèles couchés ou tête en bas sont redressés (symétrie gauche/droite, envergure
  des bras en T, tête/pieds) ; les fichiers Y-up gardent leur sens d'origine (une cape ou une traîne ne les fait
  plus tourner de dos). Vérifié sur 11 modèles variés : 11/11 debout et de face (avant : 7/11).
- Bouton « ⇅ Flip » (onglet Modèle) pour retourner à la main un modèle tête en bas.
- Visage/cou avec « Head » agrandi : la tête grandit maintenant depuis l'articulation de la mâchoire (le menton ne
  s'enfonce plus dans le col, le cou garde sa longueur) et le cou s'épaissit à moitié avec la tête ; le V-shape
  s'arrête aux aisselles et n'élargit plus la base du cou.
### Ajouté
- Curseur « V-shape » (onglet Squelette) : élargit la poitrine, hauteur par hauteur, jusqu'au côté intérieur du
  bras baissé de la base ; la taille et ce qui est déjà assez large (veste évasée) ne bougent pas. Comble le vide
  entre le torse et les bras en pose du jeu. Activé par « Disney Infinity style » et « Match the base ».
### Corrigé
- Onglet Squelette : le bloc des proportions et ses boutons sortaient de l'écran sans pouvoir défiler ; ils sont
  maintenant dans la zone qui défile, qui prend toute la hauteur disponible.

## [0.4.0] - 2026-10-17
### Ajouté
- Bouton « Disney Infinity style » : proportions moyennes de 30 personnages humanoïdes du jeu (tête, torse, bras,
  avant-bras, jambes) appliquées en un clic ; « Match the base » conservé ; bouton « Reset ».
- Curseurs « Head » (taille de la tête autour du cou) et « Forearms » (avant-bras séparés des bras).
- Curseur « Shoulder roundness » (onglet Rig) : mélange en direct entre épaules rabattues (vestes, manches larges)
  et épaules rondes (bras nus) sur le rig construit.
- Case « Preview in game pose » (onglet Squelette) : le modèle bras baissés, comme une fois installé, mis à jour
  pendant le réglage des points du squelette.
- Épaisseur du torse (curseur « Torso », suggestion d'après la base, plafonnée à ×1,4).
- Aperçu en direct de l'épaisseur des membres dans la vue 3D, avant la construction du rig.
- Aperçu en direct des filtres de texture (Game look, luminosité) sur le modèle et dans la fenêtre de l'atlas, avec
  une case « Show the original » pour comparer.
### Modifié
- Poids des doigts décochés par défaut (expérimental) : les prises du jeu froissaient les mains ; une main de moins
  de 60 sommets reste toujours rigide.
- Filtre de texture environ 2,5× plus rapide en pleine résolution (filtre guidé rapide), calculé en arrière-plan.
### Corrigé
- Texte de suggestion de l'épaisseur caché derrière le bouton « Match the base ».
- Fausse alerte « Mostly black » dans l'aperçu de l'atlas : les statistiques ne comptent plus les zones vides.

## [0.3.0] - 2026-10-16
### Ajouté
- Épaisseur des membres (onglet Squelette, curseurs bras/jambes, bouton « Match the base »).
- Outil à part `Audit_Animations.bat` : joue toutes les animations sur le rig, rapport texte/JSON + images des pires
  images.
- Filtre de texture « Game look » et calage de la luminosité sur la base.
- Icône du personnage (photo de la tête). Expérimental : la grille de sélection ne l'utilise pas encore.
- Exclusion des accessoires (cape, queue, arme) : ignorer ou retirer.
### Modifié
- Pastille verte quand un point du squelette réglé à la main correspond à la base.
- Les doigts suivent la main dans les animations d'aperçu.
### Corrigé
- Chaussures séparées : fondu avec les poids du bas du pantalon (pieds qui se détachaient).
- Plantage des curseurs de placement après la construction du rig.
- Champ de saisie et texte du curseur « Game look » désynchronisés.

## [0.2.0] - 2026-10-15
### Ajouté
- Lecteur `.dae` intégré, import `.dae` dans le nettoyage Blender.
- Poids des doigts, pose « Poing », champs de saisie à côté des curseurs, repère du coude, pastilles d'état.
- Liste complète des bases (test à la demande, filtre), 4 poses de test plus dures.
- Suggestion de bases refaite (silhouette, diversité, déformation au bake).
- Transfert robuste des poids (NumPy/SciPy, sans add-on).
- Budget de triangles au nettoyage (8000 par défaut).
### Corrigé
- Cou trouvé au bout des cheveux, hauteur d'entrejambe sur les manteaux longs.
- Textures noires (archive de textures activée par défaut), GPU à 99 % avec la base Iron Fist.
- Script d'entrée qui écrivait dans la table Crabe protégée.

## [0.1.0] - 2026-10-15
- Import du projet Disney Infinity Creator.

[Non publié]: https://github.com/Nabile01/DisneyCreator/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/Nabile01/DisneyCreator/releases/tag/v0.4.0
[0.3.0]: https://github.com/Nabile01/DisneyCreator/releases/tag/v0.3.0
[0.2.0]: https://github.com/Nabile01/DisneyCreator/releases/tag/v0.2.0
[0.1.0]: https://github.com/Nabile01/DisneyCreator/releases/tag/v0.1.0
