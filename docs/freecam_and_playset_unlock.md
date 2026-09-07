# Guide Technique : Freecam & Déblocage Tous Personnages en Play Set

Ce guide détaille la procédure technique complète pour :
1. **Implémenter une Freecam (caméra libre)** contrôlable depuis le Mod Menu de CrabeLoader.
2. **Débloquer l'utilisation de TOUS les personnages** (Disney, Marvel, Star Wars cross-era) dans les packs aventures scénarisés (**Play Sets** comme *Rise Against the Empire*, *Twilight of the Republic*, etc.).

---

## PARTIE 1 : Procédure d'implémentation de la Freecam

### 1. Pourquoi la Freecam est impossible en pur Lua ?
Dans Disney Infinity 3.0, la caméra de jeu est gérée directement en C++ par le moteur Avalanche :
- Chaque frame (60 fois/sec), le moteur calcule la position $(X, Y, Z)$ et la matrice de vue de la caméra en fonction du joueur.
- Si un script Lua tente d'écrire une position, le moteur C++ la réécrase instantanément à la frame suivante.
- Les touches du clavier (ZQSD) sont directement routées vers l'avatar.

**La solution :** Une Freecam propre se compose d'un hook C++ dans CrabeLoader, piloté par un interrupteur dans le Mod Menu Lua.

---

### 2. Architecture de la Freecam

```
┌─────────────────────────────────────────────────────────────┐
│                 CrabeLoader (bink2w32.dll)                  │
│                                                             │
│   [ Menu Lua (15_menu.lua) ]                                │
│          │ Crabe.Freecam.toggle()                           │
│          ▼                                                  │
│   [ Freecam Module (C++) ]                                  │
│          │                                                  │
│          ├─► 1. InputHook: Bloque les touches pour l'avatar │
│          │                 Lit ZQSD / Souris pour la caméra │
│          │                                                  │
│          └─► 2. MinHook: Intercepte UpdateCamera()          │
│                          Empêche le jeu d'écraser la caméra │
│                          Injecte les nouvelles coordonnées  │
└─────────────────────────────────────────────────────────────┘
```

---

### 3. Les 4 étapes de réalisation

#### Étape A : Trouver la fonction de caméra en mémoire (Cheat Engine)
1. Lancer le jeu avec Cheat Engine.
2. Chercher les coordonnées de la caméra (type `float`) en observant les valeurs qui bougent quand on oriente la caméra avec la souris.
3. Poser un breakpoint d'écriture (*"Find out what writes to this address"*).
4. Relever l'adresse de la fonction d'assemblage (par exemple `Camera::UpdateViewMatrix` ou l'instruction `movups [esi+0x20], xmm0`).

#### Étape B : Créer le Hook C++ dans CrabeLoader
Créer une classe selon les standards SOLID (`include/loader/freecam.hpp` et `src/freecam.cpp`) :

```cpp
class Freecam
{
public:
    static Freecam& get();

    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setSpeed(float speed);

    // Appelé à chaque frame de rendu (dans RenderHook ou InputHook)
    void update(float deltaTime);

private:
    bool _enabled = false;
    float _speed = 10.0f;
    Vector3 _cameraPos;
    Vector3 _cameraRot; // Pitch, Yaw
};
```

Dans le hook MinHook :
- Si `_enabled == false` : appeler la fonction originale du jeu.
- Si `_enabled == true` : ne **pas** appeler la fonction originale, mais écrire directement les coordonnées de `Freecam` dans la structure caméra du jeu.

#### Étape C : Gérer les contrôles de vol
Dans `src/input_hook.cpp`, lorsque `Freecam::get().isEnabled()` est vrai :
* **Bloquer l'envoi des touches de déplacement à l'avatar** (pour que le personnage ne bouge pas).
* **Mettre à jour la caméra :**
  - `Z` / `S` : Avancer / Reculer dans la direction où regarde la caméra.
  - `Q` / `D` : Strafe Gauche / Droite.
  - `Espace` / `Ctrl` : Monter / Descendre verticalement.
  - `Shift` : Multiplicateur de vitesse (mode rapide).
  - `Mouvement souris` : Mettre à jour les angles Pitch et Yaw.

#### Étape D : Exposer le contrôle en Lua & dans le Mod Menu
Dans `src/lua_runtime.cpp`, enregistrer les fonctions pour Lua :
```cpp
// Crabe.Freecam.set(bool)
// Crabe.Freecam.setSpeed(float)
```

Puis dans votre modmenu Lua :
```lua
Crabe.Menu.registerInCategory("CAMERA", {
    label = "Freecam",
    toggle = true,
    state = false,
    onToggle = function(enabled)
        Crabe.Freecam.set(enabled)
        return enabled and "Freecam ACTIVE (ZQSD + Souris)" or "Freecam DESACTIVEE"
    end
})

Crabe.Menu.registerInCategory("CAMERA", {
    label = "Vitesse Freecam",
    cycle = { "Lente (5)", "Normale (15)", "Rapide (40)" },
    index = 2,
    onCycle = function(label, index)
        local speeds = { 5.0, 15.0, 40.0 }
        Crabe.Freecam.setSpeed(speeds[index])
        return "Vitesse: " .. label
    end
})
```

---

## PARTIE 2 : Débloquer tous les personnages dans les Play Sets (Star Wars / Rise Against the Empire)

Dans Disney Infinity, les packs aventures scénarisés (Play Sets) imposent une restriction stricte appelée **"Franchise Lock"** :
* Dans *Rise Against the Empire* ou *Twilight of the Republic*, le jeu refuse tous les personnages non Star Wars (Mickey, Elsa, Iron Man, etc.).
* Même entre ères Star Wars, le jeu exigeait historiquement des "Champion Coins" pour autoriser des personnages comme Luke dans la prélogie.

---

### 1. Comment le jeu applique ce verrouillage ?

Le verrouillage s'exécute à deux niveaux :
1. **Niveau UI (Interface Lua / Grille de sélection) :**
   Le fichier `virtualreaderpc_gridcharacter.lua` filtre les personnages affichés ou les grise avec un état "invalide" en appelant `VirtualReaderPC_GetBrandFromCurrentPlaySet()` et `VirtualReaderPC_PlaysetCheckInGame()`.
2. **Niveau Moteur (C++) :**
   Lors du chargement de l'avatar (`VirtualReaderPC_SetCurrentCharacter`), le moteur C++ vérifie si l'ID du personnage correspond à la liste des personnages autorisés pour le pack actuel. S'il n'est pas autorisé, le spawn est avorté.

---

### 2. Procédure pour tout débloquer via CrabeLoader

Contrairement aux mods anciens qui modifiaient les fichiers `.exe` sur le disque, CrabeLoader peut lever la restriction **dynamiquement en mémoire** :

#### Étape 1 : Débloquer l'interface de sélection (Lua)
Le fichier `src/api/12_virtualreader.lua` de CrabeLoader contient déjà une fonction `patchGridCharacter()` qui force le statut `available` :

```lua
-- Dans src/api/12_virtualreader.lua
function cls:GetGridButtonState(info)
    local state = "available"
    if info.IsDead then
        state = "dead"
    end
    return state
end
```

Pour que **tous** les personnages apparaissent dans la grille même en Play Set :
- Dans `src/api/12_virtualreader.lua`, patcher la méthode de filtrage de la grille (`cls.FilterByPlayset` ou `cls.PopulateGrid`) afin qu'elle n'élimine aucun personnage de la table `VirtualReaderPC_Data.AvatarData`.

#### Étape 2 : Bypasser la vérification native C++ (`PlaysetCheck`)
Le moteur C++ appelle une fonction de validation de franchise (similaire à `FindPlaysetForCharacter` découverte dans les recherches d'ingénierie inverse).

Dans CrabeLoader :
1. **Hooker la native de validation :**
   Intercepter la fonction C++ derrière `VirtualReaderPC_PlaysetCheckInGame` via MinHook.
2. **Forcer le retour positif :**
   Faire en sorte que cette fonction retourne systématiquement `true` (personnage autorisé), quel que soit le Play Set en cours :
   ```cpp
   // Pseudo-code du hook
   BOOL __cdecl Hooked_PlaysetCheckInGame(int characterId, int playsetId)
   {
       // Au lieu de vérifier si c'est Star Wars :
       return TRUE; // Autorise TOUS les personnages (Mickey, Marvel, etc.)
   }
   ```
3. **Résultat immédiat :**
   Dès que vous sélectionnez n'importe quelle figurine dans le menu, le moteur charge le modèle 3D et ses animations dans le Play Set sans message d'erreur.

---

### 3. Intégration dans le Mod Menu

Pour offrir un contrôle total au joueur :
```lua
Crabe.Menu.registerInCategory("GAMEPLAY", {
    label = "Crossover Universel Play Sets",
    toggle = true,
    state = true,
    onToggle = function(enabled)
        Crabe.SetPlaysetCrossoverUnlock(enabled)
        return enabled and "Tous persos autorises en Play Set !" or "Restrictions normales"
    end
})
```

---

## Résumé de la démarche

1. **Freecam :** Nécessite d'intercepter la fonction de mise à jour de la caméra en C++ via MinHook pour détacher les coordonnées du joueur et les piloter au clavier/souris.
2. **Tous les persos en Play Set :** Nécessite de lever le filtre dans `12_virtualreader.lua` et de forcer la fonction de validation C++ à toujours répondre `true`.

