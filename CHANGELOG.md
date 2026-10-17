# Journal des versions

Format : [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/), versions [SemVer](https://semver.org/lang/fr/).
Commits : `type(zone): résumé` (types `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `chore`), avec un corps qui
dit pourquoi et comment c'est vérifié. Publier une version : mettre à jour `VERSION` et ce journal ; GitHub
Actions crée alors l'étiquette et la release.

## [Non publié]
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
