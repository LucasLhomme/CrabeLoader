# Architecture Multijoueur & Audit du Serveur Communautaire

Ce document présente l'audit complet du serveur communautaire situé dans `tools/disney-infinity-community-server`, suivi de l'étude de faisabilité et de la feuille de route technique pour restaurer proprement le multijoueur de **Disney Infinity 3.0 Gold Edition** via **CrabeLoader**.

---

## 1. Contexte & Problématique

Lors de la fermeture des services en ligne de Disney en 2016-2017, Disney a publié les versions **Gold Edition** sur Steam. Pour cette sortie :
- Les serveurs officiels ont été coupés.
- Les menus multijoueur ont été masqués et verrouillés dans l'interface du jeu.
- Le moteur réseau sous-jacent d'Avalanche Software est resté compilé dans l'exécutable `DisneyInfinity3.exe`.

Le dossier `tools/disney-infinity-community-server` contient une tentative communautaire (dépôt `starbrightlab`) visant à recréer ce backend.

---

## 2. Audit & Review de `disney-infinity-community-server`

### 2.1. Architecture & Stack
* **Technologies :** Node.js 18+, Express, Socket.IO, PostgreSQL / Supabase, Winston, Helmet, Multer.
* **Double interface :**
  1. Une API Web moderne (`/api/v1/*`) destinée à une application ou un site web.
  2. Une couche d'émulation des endpoints historiques du jeu :
     - `routes/config.js` : Annuaire de découverte `/coregames/config/v1/infinity3/:platform/`.
     - `routes/infinity-api.js` & `routes/disney-ugc.js` : UGC et profils.
     - `routes/did-compat.js` : Authentification Disney ID (`/coregames/did/v3/*`).
     - `routes/sessions-compat.js` : Gestion des sessions de jeu (`/coregames/sessions/v1/*`).

---

### 2.2. Ce qui est valide et fonctionnel
- **Découverte de configuration (Config Discovery) :**
  L'implémentation dans `routes/config.js` est exacte. Au lancement, le client de jeu interroge impérativement `/coregames/config/v1/infinity3/steam/` (ou `/pc/`, `/wiiu/`) pour connaître les URLs de tous les sous-services (`url_inf_ugc`, `url_cg_matchmaking`, `domain_cg_natneg`, etc.). Sans réponse JSON valide à cet appel, le jeu se verrouille en mode hors-ligne.
- **Gestion des Toyboxes (UGC) :**
  La gestion des fichiers binaires de Toybox (upload multipart, métadonnées, screenshots) reflète correctement le format attendu par le jeu.
- **Sécurité applicative de base :**
  Rate-limiting par IP/catégorie, hachage bcrypt des mots de passe, en-têtes de sécurité via Helmet.

---

### 2.3. Défauts majeurs et Bugs critiques

| Problème | Fichiers concernés | Impact |
| :--- | :--- | :--- |
| **Crash Runtime DB (`pool.query`)** | `controllers/profile.js`, `controllers/achievements.js`, `controllers/sync.js`, `controllers/analytics.js`, `services/achievementService.js` | Lors de la migration vers le client Supabase, l'export `pool` a été supprimé de `config/database.js`. L'appel `pool.query()` lève immédiatement une exception `TypeError: Cannot read properties of undefined (reading 'query')`. |
| **Données fantômes (Stubs SQL)** | `config/database.js`, `socket.js`, `controllers/friends.js` | La fonction de compatibilité `query()` renvoie `{ rows: [], rowCount: 0 }` pour toute requête SQL autre que `SELECT NOW()`. Les insertions d'amis (`INSERT INTO friends`) ou mises à jour de requêtes ne sont jamais exécutées. Les données sont perdues silencieusement. |
| **Incompatibilité Moteur (WebRTC / Socket.IO)** | `controllers/networking.js`, `socket.js` | Le serveur implémente des échanges de candidats ICE WebRTC et des événements Socket.IO. **Disney Infinity 3.0 n'utilise ni WebRTC ni Socket.IO**. Le jeu C++ communique via des sockets UDP natives (NAT negotiation Avalanche/GameSpy) et HTTP brut. |
| **Configuration client fictive** | `client-integration/Infinity3Config.xml` | Ce fichier XML n'a jamais été lu par le jeu. Le moteur compile ses URLs de base en dur dans le binaire ; il n'existe aucun mécanisme natif de chargement de config XML externe pour le réseau. |

**Bilan du serveur :**
En l'état, le serveur ne permet pas de jouer en multijoueur. Il peut servir de base pour héberger des Toyboxes (UGC), mais son architecture temps-réel (Socket.IO / WebRTC) et ses bugs de base de données bloquent toute session de jeu réelle.

---

## 3. Analyse du Moteur Réseau dans le Binaire PC

Le binaire `DisneyInfinity3.exe` (Win32) contient toujours la totalité des fonctions natives de jeu en réseau d'Avalanche Software :

### 3.1. Fonctions d'état et d'autorisation
* `IsMultiplayerAllowed` : Retourne un booléen indiquant si le mode multijoueur est autorisé (désactivé par défaut dans la Gold Edition).
* `IsOnline`, `IsOnlineContentAllowed` : Vérification du statut de connexion réseau.
* `IsSignedIntoDisneyID`, `IsSignedIntoPlatform` : Vérification de l'authentification.
* `UI_IsSteamGame`, `UI_IsTOGOOfflineGame` : Drapeaux de configuration de la build.

### 3.2. Fonctions de session et de réplication
* `Network_StartGettingList`, `Network_SetJoinSession`, `Network_SetSessionName`, `Network_RemoveFromGettingList` : Gestion des sessions réseau de bas niveau.
* `UI_LockGame`, `UI_UnlockGame`, `UI_GameIsLocked` : Verrouillage/déverrouillage de la session hôte (cf. `src/api/22_system.lua`).
* `UI_KickPlayer`, `Players_NumPlayers`, `Players_MaxPlayers` : Gestion des pairs connectés dans l'instance.

---

## 4. Faisabilité d'un Mod Multijoueur via CrabeLoader

### 4.1. Pourquoi CrabeLoader est la solution idéale
Les approches communautaires traditionnelles obligent les joueurs à :
1. Modifier leur fichier système `C:\Windows\System32\drivers\etc\hosts`.
2. Installer des certificats SSL tiers sur leur OS pour intercepter les flux HTTPS.
3. Patché manuellement les octets du binaire `DisneyInfinity3.exe` (ce qui casse les signatures et complique les mises à jour).

**CrabeLoader résout ces trois problèmes** car il s'exécute directement dans l'espace mémoire du jeu via le proxy `bink2w32.dll` avec [MinHook](file:///home/crabe/Dev/Perso/CrabeLoader/src/minhook).

---

### 4.2. Feuille de route technique pour CrabeLoader

```
┌─────────────────────────────────────────────────────────────┐
│                       CrabeLoader                           │
│                                                             │
│  ┌───────────────────────┐       ┌───────────────────────┐  │
│  │   Lua Hooks / API     │       │   C++ Native Hooks    │  │
│  │  - Déverrouillage UI  │       │  - Hook Winsock/HTTP  │  │
│  │  - Menu Multijoueur   │       │  - Bypass Auth/Flags  │  │
│  └───────────┬───────────┘       └───────────┬───────────┘  │
└──────────────┼───────────────────────────────┼──────────────┘
               │                               │
               ▼                               ▼
       Moteur Disney Infinity 3.0    Redirection Réseau
       - Logique de réplication     - Option A: Serveur local
       - Physique & Objets Toybox    - Option B: Steamworks P2P
```

#### Étape 1 : Reroutage Réseau en Mémoire (C++)
Au lieu de modifier le fichier `hosts` de Windows :
- Ajouter dans CrabeLoader un hook sur les fonctions réseau Win32 (`connect`, `getaddrinfo`, ou l'API HTTP du jeu).
- Quand le jeu initie une connexion vers `disney.go.com` ou `api.disney.com`, CrabeLoader redirige automatiquement le socket vers l'adresse locale (`127.0.0.1:3000`) ou le serveur communautaire dédié.

#### Étape 2 : Déverrouillage des Drapeaux & UI (Lua + Mémoire)
- Hooker les natives de vérification :
  - Forcer `IsMultiplayerAllowed()` à retourner `true`.
  - Forcer `IsOnline()` et `IsSignedIntoDisneyID()` à retourner `true`.
- Réactiver les entrées de menus masquées dans l'interface (via `src/api/15_menu.lua` et les scripts de menus originaux).

#### Étape 3 : Matchmaking & Transport (Deux options)

##### Option A : Via Serveur Dédié Corrigé (Legacy Avalanche)
- Corriger `tools/disney-infinity-community-server` pour réparer la base de données et l'émulation sessions/matchmaking.
- Mettre en place un serveur de négociation NAT UDP compatible avec le champ `domain_cg_natneg` renvoyé par la configuration.
- Les clients s'échangent leurs adresses IP/ports et le moteur Avalanche établit sa connexion P2P native.

##### Option B (Recommandée) : Intégration Directe Steamworks P2P
- Disney Infinity 3.0 Gold intègre déjà `steam_api.dll`.
- En interceptant les appels de session (`Network_SetJoinSession`, etc.) pour les brancher sur les APIs de lobby Steam (`SteamMatchmaking()->CreateLobby`, `JoinLobby`, invitations d'amis Steam) :
  - **Aucun serveur externe n'est nécessaire** pour le multijoueur direct entre amis.
  - La traversée NAT est gérée directement par les relais Steam (Steam Datagram Relay).

---

## 5. Synthèse & Prochaines Actions

1. **Ne pas utiliser `disney-infinity-community-server` pour le temps réel en l'état :**
   Le serveur nécessite une refonte de sa couche SQL et de son modèle réseau (suppression de Socket.IO/WebRTC au profit de sockets adaptés au jeu).
2. **Implémenter les bases dans CrabeLoader :**
   - Créer un module C++ de hook pour dévier les URLs du jeu en mémoire.
   - Forcer le retour des natives d'authentification et de permission multijoueur.
   - Tracer avec Wireshark et les logs du loader les paquets émis lorsque le jeu tente de créer ou rejoindre une session.

