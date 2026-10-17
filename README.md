# Disney Infinity Creator

**Français** · [English](README.en.md)

Transforme n'importe quel modèle 3D en **personnage jouable** pour **Disney Infinity 3.0 Gold Edition (PC)**.

Tu importes un modèle (Sketchfab, Models Resource, ton propre travail Blender…), tu choisis une figure du jeu comme
base, et Creator ajuste un squelette, calcule le skinning, te laisse prévisualiser le résultat en mouvement puis
l'installe sous forme de mod. Le personnage apparaît dans le jeu comme une **nouvelle figure à part entière** : il
reprend les animations, les attaques et les capacités de la figure de base, et aucun personnage d'origine n'est
remplacé.

> Outil de fan, sans lien avec Disney, Avalanche Software ni aucun ayant droit. Il faut posséder sa propre copie du jeu.

---

## Sommaire

- [Fonctionnalités](#fonctionnalités)
- [Prérequis](#prérequis)
- [Installation et lancement](#installation-et-lancement)
- [Créer un personnage pas à pas](#créer-un-personnage-pas-à-pas)
- [Comprendre le score de déformation](#comprendre-le-score-de-déformation)
- [Gérer ses personnages](#gérer-ses-personnages)
- [Outils en ligne de commande](#outils-en-ligne-de-commande)
- [Jouer avec son personnage](#jouer-avec-son-personnage)
- [Conseils pour un bon résultat](#conseils-pour-un-bon-résultat)
- [Fonctionnement](#fonctionnement)
- [Limites connues](#limites-connues)
- [Dépannage](#dépannage)
- [Pour les développeurs](#pour-les-développeurs)
- [Crédits](#crédits)

---

## Fonctionnalités

- **99 figures de base** : tous les personnages jouables de Disney Infinity 3.0 (Marvel, Star Wars, Disney, Pixar,
  y compris ceux des versions 1.0 et 2.0), avec recherche et aperçu 3D texturé.
- **Rien n'est écrasé** : chaque création est une nouvelle figure avec sa propre tuile dans la grille. Luke, Thor ou
  Elsa restent intacts, et tu peux installer autant de personnages que tu veux.
- **Nettoyage intégré** : à l'import, Blender (en arrière-plan) fusionne les morceaux, corrige l'échelle et
  l'orientation, retrouve les textures perdues (fichier `.mtl` absent, images à côté du modèle) et fige un éventuel
  squelette. Le résultat est un fichier temporaire supprimé à la fermeture : aucun fichier parasite chez toi. Les
  `.fbx` passent toujours par ce nettoyage.
- **Placement manuel** : décale et tourne le modèle (curseurs, boutons de 1 cm, rotation fine, « Recentre ») quand
  l'ajustement automatique se trompe.
- **Vue 3 plans** (face, profil, dessus) pour juger l'alignement du squelette d'un coup d'œil.
- **Rigging automatique** : épaules, coudes, poignets, cou, hanches, genoux et chevilles sont détectés, avec un
  curseur par articulation pour affiner.
- **Poids de skinning de qualité** via Blender s'il est installé. S'il échoue (maillages ouverts, nombreux morceaux)
  ou s'il est absent, Creator bascule seul sur une méthode par distance aux os, avec des règles par zone.
- **Aperçu en mouvement** : pose en jeu, bras levés, marche, avec un curseur d'amplitude (« Pose amount »).
- **Score de déformation** sur 100, avec surlignage en rouge des arêtes trop étirées, et détection d'un modèle
  « effondré » quand les poids sont cassés.
- **Suggestion de bases** : classe les figures de base selon leur compatibilité avec ton modèle.
- **Verrouillage de pièces** : Ctrl+clic sur une pièce (cheveux, cape, arme…), « Grow » pour prendre les petites
  pièces voisines, « Lock » pour la forcer à suivre un seul os.
- **Aperçu de l'atlas de textures** avec contrôles (image trop sombre, non carrée…), et avertissements à l'import
  (matériau sans texture, texture répétée).
- **Icônes en jeu** : portrait du HUD et figure de la grille de sélection, photographiés automatiquement sur ton
  modèle.
- **Relief** : une normal map tirée de la texture donne l'aspect « figurine sculptée » des personnages du jeu.
- **Mod partagé léger** : textures et icônes de tous tes personnages dans un seul mod (`zz_creator_shared`), au lieu
  d'une archive de ~13 Mo par personnage ; `Gerer_Mods.bat` liste, désactive et supprime proprement tes personnages.
- **Journal d'erreurs** (`build/creator.log`) pour comprendre un plantage.
- **Installation vérifiée** : chaque installation est contrôlée (tampons, poids, liens de fichiers) et l'ancienne
  version du mod est sauvegardée dans `backups/`.

## Prérequis

| Quoi | Remarques |
| :--- | :--- |
| Windows 10 ou 11 | |
| Disney Infinity 3.0 Gold Edition (PC) | Ta propre copie installée |
| [CrabeLoader](https://github.com/LucasLhomme/CrabeLoader) et [CrabeMenu](https://github.com/LucasLhomme/CrabeMenu) | Le chargeur de mods qui charge ton personnage. CrabeMenu (menu `F5` en jeu) est conseillé pour passer sur ton personnage |
| Python 3.12 ou plus récent | Cocher « Add python.exe to PATH » à l'installation |
| Blender 4.x ou 5.x | Recommandé : nettoyage des modèles (obligatoire pour `.fbx`) et meilleurs poids. Creator le lance en arrière-plan, tu n'as jamais à l'ouvrir |

## Installation et lancement

1. Télécharge ou clone ce dépôt.
2. Double-clique sur **`Launch_Creator.bat`**. Au premier lancement, il crée l'environnement virtuel `.venv` et
   installe les dépendances (`customtkinter`, `moderngl`, `numpy`, `Pillow`, `trimesh`, `scipy`, `cryptography`),
   puis ouvre Creator. Les fois suivantes, il démarre directement.
3. Si l'appli s'arrête, la fenêtre reste ouverte et renvoie vers `build\creator.log`.

> `run.ps1` (PowerShell) fait la même chose et réinstalle les dépendances à chaque lancement : utile après une mise
> à jour de Creator.

Le chemin de Blender et celui du jeu se règlent dans l'onglet **Rig & Export** ; ils sont mémorisés.

## Créer un personnage pas à pas

Le panneau de gauche a cinq onglets ; on les parcourt dans l'ordre.

### 0. Base : choisir la figure

Choisis la figure dont le squelette, les animations et le moveset serviront à ton personnage.

| Marque | Sens |
| :---: | :--- |
| ● | Squelette standard : toutes les articulations nécessaires existent |
| ◐ | Squelette adapté : quelques os en remplacent d'autres (ex. la base du pied pour la cheville). Fonctionne, avec de petites approximations |
| ✕ | Non humanoïde (Olaf, Randall…) : ne peut pas recevoir un maillage humanoïde |

**Choisis une base de gabarit proche.** Un héros mince va mieux sur Spider-Man que sur Hulk, une longue robe mieux
sur Elsa que sur Captain America. Le bouton **« Suggest the best bases for my model »** (onglet Modèle) classe les
figures pour toi : il compare la silhouette de ton modèle à celle de chaque figure (mise en T-pose), garde des types de
corps différents (pas cinq clones), puis construit le vrai rig des meilleures et mesure l'étirement dans les poses de
test et la déformation au moment de caler ton T-pose sur la pose du jeu. Chaque ligne affiche ces trois mesures.

Comme le moveset vient avec la figure, **toute la liste (~90 figures) reste affichée** : les 12 meilleures sont testées
d'office, les autres sont classées par silhouette (« not tested »). Le bouton **« Test the next 12 figures »** teste
le lot suivant, et le champ de filtre cherche par nom ou par licence (Avengers, Star Wars…).

**Doigts (expérimental, décoché par défaut).** Les figures de base ont des os de doigts (Thumb1-3, Index1-2,
Middle1-2). Avec la case **« Finger weights »**, la main de ton modèle suit ces os : chaque sommet au-delà des
articulations des doigts est partagé entre le poignet et la chaîne de doigts la plus proche. Tant que les doigts du
modèle ne tombent pas sur ceux de la base (4 doigts contre 3 chaînes, mains plus grandes), les prises du jeu (sabre,
poing) froissent la main : la main rigide est donc le réglage par défaut. Une main de moins de 60 sommets reste toujours
rigide. La pose **Fist** du sélecteur montre le résultat.

**Format `.dae` (COLLADA).** Creator le lit lui-même, sans Blender : géométrie, UV, textures (cherchées à côté du
fichier), unité et axe vertical. Un modèle skinné est lu dans sa pose de repos. Avec la case de nettoyage cochée,
Creator le convertit en `.glb` (couleurs identiques) avant de le confier à Blender, qui n'a plus d'import Collada
depuis la version 5.

**Rendu du jeu (filtre de texture).** Onglet Modèle, curseur « Game look » (0 = texture d'origine). Les modèles de fans
sont plus sombres, plus ternes et pleins d'ombres peintes ; les personnages du jeu sont clairs, saturés et lisses. Le
filtre adoucit le bruit et les ombres cuites (filtre guidé qui préserve les contours), relève les ombres et ajoute de la
saturation. Il ne travaille qu'à l'intérieur des îlots UV : aucune couleur ne bave depuis le fond. Le résultat
s'affiche en direct dans la vue 3D et dans « Preview the texture atlas » (version rapide en 512 px pendant que le curseur
bouge, pleine résolution au relâchement) et c'est celui qui est exporté dans le jeu. Le second curseur « Match the base figure's
brightness » rapproche la luminosité moyenne de celle de la texture de la base choisie (0 = désactivé).

**Relief.** Case « Relief » (onglet Rig & Export, activée par défaut) : Creator tire une normal map de la texture
(coutures, plis et bords de couleur légèrement creusés) au format des cartes du jeu, comme les figures officielles.
Décocher pour revenir à une carte plate.

**Icônes du personnage.** Case « Character icons » (onglet Modèle) : à l'installation, Creator photographie le
modèle (texture avec le filtre, pose de jeu une fois riggé) et écrit les images au format des fichiers du jeu :
le portrait HUD 128 px (`ui/icons/hud_playericons_<acteur>.mtb` + texture, nommé dans `mod.json` par
`"icon": "HUD_PlayerIcons_<Acteur>"`) et la figure du Virtual Reader (`ui/virtualreadercharacters/<acteur>.mtb`,
`_sm`, et la silhouette `_s`). `<acteur>_icon.png` reste à la racine du mod. Le bouton « Preview » montre les trois
images. Le jeu ne lit ces images que dans ses archives : Creator les ajoute à celles du mod partagé (voir
[Gérer ses personnages](#gérer-ses-personnages)).

**Audit des animations (outil à part, hors Creator).** Glisse un modèle sur `Audit_Animations.bat` (base par défaut
TCW_Anakin ; une autre en 2ᵉ argument). L'outil construit le rig, joue toutes les animations dessus et écrit dans
`build\audit\<modèle>\` : `report.txt` (score statique + une ligne par animation : part d'arêtes déformées de plus de 2×,
pire image, zones touchées), `report.json` et des images de la pire image des animations problématiques (arêtes
étirées en rouge). Les clips sont les mouvements procéduraux de Creator, pas les animations d'origine du jeu. Les doigts
suivent la main dans ces clips (avant, ils restaient en arrière et déchiraient les mains à l'écran).

**Proportions (tête, torse, bras, avant-bras, jambes).** Onglet Squelette, cinq curseurs : « Head » (×0,8 à ×1,5,
la tête grandit depuis la mâchoire, le menton reste en place, le cou s'épaissit à moitié), « Torso », « Arms », « Forearms » et « Legs » (×0,8 à ×1,8,
gonflés autour de leur axe sans changer de longueur, en fondu avec les poids). Aperçu en direct, appliqué au rig.
- **Disney Infinity style** : rapproche le modèle des proportions moyennes de 30 personnages humanoïdes du jeu (mesurées
  dans le jeu : tête = 21 % de la hauteur, avant-bras plus épais que le bras, etc.). Jamais plus petit que le modèle,
  plafonds : tête ×1,4, torse ×1,4, membres ×1,6.
- **Match the base** : copie l'épaisseur de la base choisie (sans toucher à la tête).
- **Reset** : revient aux proportions du modèle.
Un vêtement ample (pantalon bouffant) compte comme épais : les jambes restent alors à ×1.
- **V-shape** (0 à 1) : en pose du jeu, les bras pendent depuis les épaules de la base (larges) ; un modèle au torse
  étroit laisse alors un vide sous les bras. Le V-shape élargit la poitrine sur les côtés, tranche par tranche, jusqu'au
  côté intérieur du bras baissé (moins 2 % de la hauteur) : plein effet de la poitrine aux aisselles, rien à la taille
  ni au-dessus des aisselles (cou, col), et une tranche déjà assez large (veste évasée) ne bouge pas. Les deux boutons le mettent à 1.

**Pose du jeu pendant le réglage du squelette.** Onglet Squelette, case « Preview in game pose » : le modèle est
montré bras baissés, comme le jeu l'affichera, et suit chaque curseur du squelette (aperçu calculé en quelques
millisecondes avec des poids approchés ; le squelette vert, placé sur la T-pose, est masqué pendant l'aperçu). Si le
rig est déjà construit, c'est sa vraie pose de jeu qui s'affiche.

**Rondeur des épaules.** Onglet Rig & Export, curseur « Shoulder roundness », actif une fois le rig construit. Les bras
tournent d'environ 80° pour passer de la T-pose à la pose du jeu. À 0 (défaut), la jonction poitrine/bras est rabattue
vers l'articulation : adapté aux vestes et manches larges (Sora). À 1, elle garde son volume (quaternions duaux +
lissage) : épaules rondes, adapté aux bras nus (Xehanort). Le mélange s'affiche en direct et c'est lui qui est exporté.

**Pastilles du squelette.** ✓ vert = proche des proportions de la base (y compris après un réglage manuel) ; ✎ bleu =
réglé à la main mais encore éloigné de la base ; ⚠ = estimé ou trop différent.

**Accessoires (cape, queue, ailes, arme).** Une cape qui pend derrière le bassin fausse les mesures (profondeur du
torse, genoux, entrejambe) et se fait déchirer par les jambes. Dans l'onglet Modèle, boîte « Accessories » :

- **Find** cherche (1) tout ce qui est plus loin derrière le corps que la profondeur réglée (curseur, 0,20 × la
  hauteur par défaut), même si la cape est soudée au corps, et (2) les pièces séparées qui dépassent. Le résultat est
  surligné en magenta. Tu peux aussi sélectionner une pièce avec Ctrl+clic.
- **Ignore** : la partie reste dans le modèle, mais n'est plus vue par l'auto-fit ni par le classement des bases, et elle
  suit le torse (hanches, buste) au lieu des jambes et des bras.
- **Remove** : la partie est retirée du modèle.
- **✕** à côté de la ligne annule l'une ou l'autre. « Lock » (pièce rigide sur un os) reste disponible pour les pièces
  séparées.

**Onglet Skeleton.** Chaque curseur a une petite note (sur quoi aligner le point vert) et une **case de saisie** à droite
pour taper la valeur exacte (Entrée valide). Une petite icône à droite de chaque point : ✓ trouvé sur le mesh, ⚠ jaune estimé par l'auto-fit,
⚠ orange loin des proportions de la base, ✎ réglé par toi ; survole-la pour lire la raison. Les curseurs en jaune « ⚠ check » sont ceux que l'auto-fit n'a fait
qu'estimer. Le curseur **Elbow position** place le coude. Sous le résumé, des avertissements ⚠ indiquent où tes
proportions s'écartent de la base de 20 % ou plus (bras, épaules, jambes) et quel curseur vérifier.

Les poses de test comptent six poses : bras levés, marche, **bras au-dessus de la tête, bras devant, coudes pliés et
torsion du buste** (40 % de la pénalité d'étirement). Elles sont aussi disponibles dans le sélecteur de pose pour
voir où les épaules cassent.

La case **« in T-pose »** (sous « Show the base figure alongside ») remet la figure de comparaison et son squelette cyan
en T-pose, pour les aligner avec ton modèle.

### 1. Modèle : importer

Clique sur **Import a 3D model** et choisis un `.obj`, `.glb`, `.gltf`, `.fbx` (ou `.dae`, `.ply`, `.stl`, `.off`),
ou un `.zip` qui le contient avec ses textures.

- La case **« Clean with Blender while importing »** est cochée par défaut. Elle lance le nettoyage décrit plus haut.
  Sans chemin Blender valide, les formats `.glb`/`.obj`/`.zip` s'importent tels quels ; `.fbx` demande
  Blender.
- Le modèle doit être en **T-pose** (bras écartés) ; une A-pose passe aussi.
- Creator détecte l'orientation. Sinon : **↻ 90°**, **↻ 180°**, **Stand up**, **⇅ Flip**.
- **Model height** est préréglée à la taille de la base. Coche **Show … alongside** pour comparer côte à côte.
- **Position** : curseurs gauche/droite, arrière/avant, bas/haut, rotation fine ; boutons ◄ 1 cm ►, ▼ back, ▲ forward ;
  **Recentre** revient au centrage automatique.
- **Pièces** : Ctrl+clic sur une pièce de la vue pour la sélectionner, **Grow** pour élargir aux petites pièces
  voisines, choisis l'os et **Lock** : la pièce suivra cet os de façon rigide (ex. des mèches de cheveux sur la tête).
- **Preview the texture atlas** montre la texture que le jeu recevra, avec des alertes.
- Les avertissements d'import (matériau sans texture, texture répétée…) s'affichent en jaune sous le nom du modèle.

### 2. Skeleton : vérifier les articulations

Points jaunes = articulations, lignes vertes = os. Creator les place seul ; le texte au-dessus des curseurs indique ce
qui est **détecté** et ce qui n'est qu'**estimé** (à vérifier en premier). Ajuste avec les curseurs (hauteur des bras,
largeur d'épaules, poignets, cou, tête, hanches, écart des jambes, genoux, chevilles). **Auto-Fit** repart du
placement automatique. Active **« 3 views »** (barre sous le viewport) pour voir face, profil et dessus.

### 3. Abilities & Moveset

Réglages des capacités et de l'ensemble de mouvements du personnage (hérités de la base).

### 4. Rig & Export : construire et installer

1. **Character name** : nom affiché dans les menus (ex. `Mario`) ; il nomme aussi le dossier du mod.
2. **Internal actor name** : nom interne du moteur (ex. `MAR_Mari`), 3 à 32 caractères, lettres/chiffres/`_`, commence
   par une lettre ; prérempli.
3. **Game folder** : ton dossier Disney Infinity 3.0.
4. **Blender** : chemin de `blender.exe` (facultatif mais conseillé).
5. **1. Build the rig**, puis contrôle **In-game pose**, **Arms up**, **Walk** et le **score de déformation**. Si
   quelque chose cloche, retourne en onglet 1 ou 2, ajuste, reconstruis.
6. **2. Install into the game** : le journal liste les contrôles, tous doivent afficher `OK`.

Le mod est écrit dans `<jeu>/mods/<nom du personnage>/`. Réinstaller sous le même nom remplace le mod et déplace
l'ancienne version dans `backups/`. Creator met ensuite à jour le mod partagé `zz_creator_shared` (textures et icônes,
quelques secondes). **Ferme le jeu avant d'installer** : il garde ses archives ouvertes.

### Option : transfert robuste des poids

Case « Robust weight transfer » (onglet Modèle). Creator construit le rig deux fois (poids Blender, puis poids
copiés depuis le corps de la figure de base) et garde celui qui déforme le moins. Utile surtout pour les jupes, capes
et manteaux longs. Aucun module à installer en plus (NumPy et SciPy suffisent).

- Méthode : « Robust Skin Weights Transfer via Weight Inpainting » (Abdrashitov, Raichstat, Monsen, Hill, SIGGRAPH
  Asia 2023), réimplémentée sans dépendance compilée. Inspiré de l'add-on Blender gratuit
  [Robust Weight Transfer](https://github.com/sentfromspacevr/robust-weight-transfer) de SentFromSpaceVR et du code
  de référence [RobustSkinWeightsTransferCode](https://github.com/rin-23/RobustSkinWeightsTransferCode) (MIT).

## Gérer ses personnages

Le jeu ne lit les textures et les icônes que dans ses propres archives (`assets/textures/17.zip`,
`assets/ui/icons/icons.zip`…), qu'un mod doit fournir entières. Pour ne pas répéter ces archives dans chaque
personnage :

- chaque personnage (`mods/<nom>/`) garde seulement ses fichiers sources (`textures/*.tbody`, `ui/…`) ;
- le mod **`mods/zz_creator_shared`** contient les archives reconstruites une fois pour tous les personnages actifs
  (les fichiers du jeu sont recopiés tels quels, seuls tes fichiers sont ajoutés). Ne le modifie pas à la main.

**`Gerer_Mods.bat`** (outil à part, jeu fermé) :

| Choix | Effet |
| :--- | :--- |
| 1. Supprimer | Envoie le personnage à la corbeille et refait les archives : il ne reste rien de lui dans le jeu |
| 2. Désactiver / réactiver | Préfixe `_` sur le dossier (ignoré par CrabeLoader), puis refait les archives |
| 3. Nettoyer | Range les personnages faits avec une ancienne version (une archive de 13 Mo chacun), propose d'envoyer les sauvegardes de `backups/` à la corbeille |
| 4. Reconstruire | Refait les archives partagées de force |

Quand le lancer : pour supprimer ou désactiver un personnage (plutôt que d'effacer son dossier), après avoir effacé un
dossier à la main, ou si l'installation signale « shared archives NOT rebuilt » (jeu ouvert). La liste indique
« Archives partagées à jour » ou ce qu'il faut reconstruire. En ligne de commande : `--list`, `--rebuild`, `--clean`.

## Comprendre le score de déformation

Après « Build the rig », Creator mesure combien le maillage s'étire ou s'écrase dans les poses de test (bras levés à
70°, marche à 35° ; le curseur « Pose amount » change l'amplitude).

Le score (0 à 100) mélange trois mesures : l'étirement moyen de toutes les arêtes (50 %), la part des arêtes
déformées de plus de 2 fois (30 %) et la part des arêtes déchirées à plus de 3 fois (20 %).

| Score | Niveau |
| :---: | :--- |
| 75 et plus | Bon |
| 50 à 74 | Acceptable |
| moins de 50 | Forte déformation |

- Les arêtes étirées de plus de 3 fois sont dessinées en **rouge** (case « Show stretched edges in red »).
- Si le modèle est **écrasé ou aplati** (poids cassés), le score affiche « FAILED - the model is collapsed ».
- Ce score mesure la géométrie, **pas l'apparence** : un modèle peut être « bon » et mal rigué (cheveux qui suivent les
  bras). Contrôle toujours visuellement, et verrouille les pièces gênantes sur le bon os.

## Outils en ligne de commande

Sans ouvrir l'interface (à lancer avec `.\.venv\Scripts\python.exe`) :

| Commande | Rôle |
| :--- | :--- |
| `tools\batch_check.py dossier [--base ACTOR \| --suggest] [--blender chemin]` | Place et rigge tous les modèles d'un dossier, affiche le score ; avec `--suggest`, essaie toutes les figures et donne les meilleures |
| `tools\capture.py modele.glb [--base ACTOR] [--amount 1.0] [--out dossier]` | Construit le rig d'un modèle et enregistre des images en 3 vues (repos, bras levés, marche, avec les arêtes étirées en rouge) et `report.txt` dans `build\captures\` |
| `Capturer_Modele.bat` | Même chose en glissant un modèle sur le fichier |
| `Audit_Animations.bat` | Audit des animations d'un modèle (rapport + images), hors Creator |
| `Gerer_Mods.bat` / `tools\manage_mods.py` | Gérer les personnages installés (voir plus haut) |

## Jouer avec son personnage

1. **Redémarre le jeu** : les nouveaux personnages ne sont chargés qu'au démarrage.
2. Dans le jeu, soit **F5** (CrabeMenu) → **Heroes › ★ Modded Characters**, soit la grille **My collection › Characters**
   où ton personnage a sa propre tuile.

## Conseils pour un bon résultat

- **Nombre de sommets** : moins d'environ 65 000 (limite du moteur, indices 16 bits). Le nettoyage décime si besoin.
- **Un maillage propre** : les petites pièces (yeux, boutons, accessoires) sont rattachées à la partie la plus
  proche, mais un modèle découpé en dizaines de morceaux se rigge moins bien.
- **Vraie T-pose** : bras droits et horizontaux, jambes légèrement écartées, de face.
- **Installer Blender** : plis plus doux aux épaules, coudes et genoux quand le calcul de chaleur réussit.
- **Proportions proches de la base** : les membres sont étirés pour coller aux longueurs d'os de la base (dans des
  limites). Des proportions très différentes donnent un personnage étiré en jeu.
- **Textures** : importe un `.zip` avec les images à côté du modèle, ou un `.glb` avec textures intégrées.

## Fonctionnement

Chaque figure jouable est un **acteur** : un squelette, un maillage skinné, des matériaux et textures, et les
animations et capacités qui le pilotent. Creator crée un **acteur cloné** : une copie de la figure de base dont le
maillage est remplacé par le tien, sous un nouveau nom.

1. **Ajustement du squelette** : des coupes horizontales du modèle donnent la ligne des bras, le cou, l'entrejambe,
   les genoux et les chevilles ; les articulations standard sont placées dessus, puis corrigées par de petits modèles
   calibrés sur les figures du jeu. Le centrage ignore cheveux et ailes
   pour que le squelette ne soit pas décalé.
2. **Correspondance des os** : chaque articulation est reliée à l'os le plus proche que la base possède vraiment ;
   une position manquante est reconstruite d'après les proportions d'une figure de référence.
3. **Skinning** : poids de chaleur (« bone heat ») de Blender, ou à défaut distance aux os. Blender échoue sur les
   maillages ouverts ou en très nombreux morceaux : si moins de 70 % des sommets reçoivent un poids, Creator utilise la
   méthode par distance. Ensuite : tête rigide (les yeux ne glissent pas), pièces sous les genoux liées à une
   cheville, pièces reliant bras et jambes suivant la colonne, lissage des panneaux entre les jambes (jupes, robes),
   os de torsion du jeu sur les bras et les cuisses, puis les pièces verrouillées à la main.
4. **Cuisson dans la pose de repos** : le maillage n'est pas rigué sur un nouveau squelette. Chaque partie du corps
   est tournée et étirée de l'articulation de ton modèle vers l'articulation correspondante de la base, si bien que
   le maillage épouse exactement la pose de repos de la base. Le squelette et les matrices de la base ne changent pas :
   **toutes les animations du jeu s'appliquent telles quelles**.
5. **Écriture des fichiers** : Creator lit et écrit directement les formats du jeu : scènes Octane (`.oct`, `.bent`),
   Lua compilé (`.dnax`), paquets de matériaux (`.mtb`), paquets d'interface et archives chiffrées, sans perte (réécrire un fichier intact redonne les mêmes
   octets).

### Contenu d'un mod de personnage

| Fichier | Rôle |
| :--- | :--- |
| `characters/<acteur>/<acteur>.oct`, `_0.vbuf`, `_0.ibuf` | Le maillage : sommets, poids, triangles, limites |
| `characters/<acteur>/<acteur>.mtb` | Les matériaux, avec textures |
| `characters/<acteur>/<acteur>.dnax`, `.bent`, `.animtreeoverrides` | La définition de l'acteur, pointant vers le nouveau maillage |
| `textures/<id>.tbody` | Textures du personnage (couleur et relief), sources de l'archive partagée |
| `ui/icons/…`, `ui/virtualreadercharacters/…`, `<acteur>_icon.png` | Icônes (sources de l'archive partagée) |
| `skilltrees/ActorList.patch`, `skilltrees/DataMap.patch` | Enregistrent l'acteur dans les bases de données du jeu |
| `characters/<Acteur>.lua` | Ajoute la figure à la grille de personnages |
| `<nom>.lua`, `mod.json` | Point d'entrée CrabeLoader et entrée CrabeMenu |

## Limites connues

- **Moveset** : le personnage reprend les mouvements, la voix, les capacités et l'arbre de compétences de sa base ;
  pas d'animations personnalisées.
- **Textures** : seule la couleur de base du modèle est utilisée (le relief est déduit de cette couleur, pas d'une
  vraie normal map du modèle). Une seule texture par personnage (atlas), plafonnée à 2048 px.
- **Figures non humanoïdes** (Olaf, Randall, Maleficent, Elastigirl) : inutilisables comme base.
- **Squelettes adaptés** (◐) : quelques articulations reconstruites à peu près.
- **Capes, jupes, cheveux** : sans « bone heat » réussi, le skinning par distance est plus approximatif ; le score ne
  voit pas tout, contrôle les poses et verrouille les pièces si besoin.
- **Placement automatique du squelette** : c'est la principale source d'écart. Refaites à travers Creator, les
  figures du jeu s'écartent d'environ 3,5 % de leur forme d'origine avec l'auto-fit, contre 1,6 % avec leurs vraies
  articulations. Vérifie surtout les épaules, les coudes et le bassin dans l'onglet Skeleton.
- **Maturité** : testé en jeu sur les bases Luke, Anakin et Gamora ; les autres bases utilisent la même chaîne mais
  ont été moins jouées.

## Dépannage

| Problème | À essayer |
| :--- | :--- |
| L'appli se ferme au lancement | Lance `Launch_Creator.bat` (la fenêtre reste ouverte) et consulte `build\creator.log` |
| Textures noires ou icône « IN » en jeu | Ferme le jeu, `Gerer_Mods.bat` → 4. Reconstruire ; l'icône n'existe que si « Character icons » était coché à l'installation |
| Le personnage n'apparaît pas en jeu | Redémarre le jeu ; vérifie CrabeLoader ; consulte `loader.log` dans le dossier du jeu |
| « model too detailed » | Réduis sous ~65 000 sommets (le nettoyage le fait) |
| Modèle couché ou de dos | Onglet 1 : **Stand up**, **↻ 90°** / **↻ 180°**, **⇅ Flip** |
| Squelette décalé par rapport au corps | Onglet 1 : curseurs de position ou **Recentre** ; vue « 3 views » pour vérifier |
| Le modèle devient une aiguille / « collapsed » | Les poids Blender ont échoué ; Creator bascule normalement sur la distance. Reconstruis le rig et lis le journal |
| Modèle gris à l'import | Lis l'avertissement jaune : texture non liée. Mets les images à côté du modèle (dans le même `.zip`) |
| Coudes ou genoux qui plient mal | Onglet 2 : bouge le curseur concerné puis reconstruis |
| Cheveux ou cape qui suivent les bras | Ctrl+clic sur la pièce, Grow, choisis l'os (ex. Head), Lock, puis reconstruis |
| Pliages raides ou en blocs | Renseigne le chemin de Blender (onglet Rig & Export) |
| Le personnage glisse sans bouger les jambes | Animations de la base absentes de l'installation : réinstalle ou vérifie les fichiers via Steam |
| « already a figure of the game » | Choisis un autre nom d'acteur interne |

## Pour les développeurs

```
src/
  core/       logique pure, sans interface
    octane_scene.py, luac.py, material_bundle.py        lecture/écriture sans perte .oct/.bent, .dnax, .mtb
    base_character.py, base_pack.py, roster.py          figures de base
    skeleton_map.py                                      articulations Creator -> os de chaque figure
    model.py, skeleton.py, rig.py, rigging.py            modèle importé, articulations, skinning
    quality.py, ranking.py                               score de déformation, classement des bases
    cleaner.py, errorlog.py                              nettoyage Blender intégré, journal d'erreurs
    octane.py, exporter.py, mod_writer.py, textures.py   sortie du mod
    icon.py, normalmap.py, twist.py                      icônes, relief, os de torsion
    shared.py, octane_zip.py                             mod partagé, archives chiffrées du jeu
  render/     viewport OpenGL hors écran (aussi utilisé par tools/capture.py)
  ui/         interface customtkinter
  scripts/    étapes Blender en arrière-plan (nettoyage, poids bone heat, figer un .glb rigué)
  assets/     bases/ (pack des figures), roster.json, shader_layouts.json, templates/
  tools/      QuickBMS et le ré-encrypteur d'archives (offsetting.exe)
tools/        batch_check.py, capture.py, audit.py, manage_mods.py, scripts qui régénèrent src/assets
              depuis une installation du jeu, outils de mesure (roundtrip.py, fit_calibration.py, twist_profiles.py)
tests/        tests unitaires (pytest)
```

Tests : `.\.venv\Scripts\python.exe -m pytest tests`.

`tools/roundtrip.py <ACTEUR>` refait une figure officielle à travers Creator et la compare à l'originale (forme,
poids, squelette) ; `--true-skeleton` et `--official-weights` isolent chaque source d'erreur. `Sync_Claude.bat`
regroupe `src/assets` en une archive pour une session de développement distante.

### Ressources du jeu

`src/assets/` et `src/tools/` (extraits du jeu, binaires tiers) ne sont **pas** versionnés ici. Ils sont régénérés
depuis une installation du jeu :

| Script | Produit |
| :--- | :--- |
| `tools/build_roster.py <virtualreaderpc_data.lua> <toybox_actors.lua>` | `roster.json` : chaque figure jouable |
| `tools/build_shader_table.py <racine décryptée du jeu>` | `shader_layouts.json` : emplacement des textures par shader |
| `tools/build_base_pack.py <dossier du jeu>` | `bases/` : modèle, `.dnax`, animations et textures d'aperçu de chaque figure |

QuickBMS (avec le script BMS Disney Infinity) et `offsetting.exe` vont dans `src/tools/`.

### Conventions

- Branche principale : `main`. Une modification = un commit avec un message clair.
- Commits `type(zone): résumé` avec un corps qui dit pourquoi ; chaque version est décrite dans `CHANGELOG.md`.
  Publier une version : mettre à jour `VERSION` et le journal, GitHub Actions crée l'étiquette et la release.
- `build/`, `backups/`, `.venv/`, `src/assets/`, `src/tools/` et `src/config/settings.json` (chemins personnels) ne
  sont pas suivis, ni les modèles 3D (`*.glb`, `*.fbx`, `*.dae`…).

## Crédits

- **QuickBMS** de Luigi Auriemma et le script BMS Disney Infinity de la communauté.
- La communauté de modding Disney Infinity (ré-encrypteur d'archives, recherches sur les formats).
- [trimesh](https://trimesh.org), [ModernGL](https://github.com/moderngl/moderngl),
  [CustomTkinter](https://github.com/TomSchimansky/CustomTkinter), NumPy, SciPy, Pillow,
  [cryptography](https://cryptography.io) et
  [Blender](https://www.blender.org).

Disney Infinity et tous les personnages associés sont des marques de leurs propriétaires respectifs. Ce projet est
destiné aux possesseurs du jeu.
