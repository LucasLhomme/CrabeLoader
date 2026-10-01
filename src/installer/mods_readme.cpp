/*
** CrabeLoader
** File description:
** Holds the text of mods/LISEZMOI.txt, written once beside the empty mods folder.
** English first, then French; the keys named are the shipped defaults in crabe.toml.
** Contains no logic: it is the readme and nothing else.
**
** Authors: @LucasLhomme
*/

#include "installer/game_installer.hpp"

namespace crabe::installer {

    std::string_view modsReadmeText() noexcept
    {
        return "CrabeLoader - README\r\n"
               "====================\r\n"
               "\r\n"
               "CrabeLoader is installed. This \"mods\" folder is ready for your mods.\r\n"
               "\r\n"
               "ADDING A MOD\r\n"
               "  Drop the mod's folder here, with its mod.json file:\r\n"
               "      mods\\mod_name\\mod.json\r\n"
               "  Then start the game. Nothing else needs to be set up.\r\n"
               "\r\n"
               "DEFAULT KEYS\r\n"
               "  Insert    Opens or closes the developer console (overlay).\r\n"
               "  F4        Reloads every mod without restarting the game.\r\n"
               "  Both can be changed in Crabe\\crabe.toml, [keybinds] section (hotReload, devOverlay).\r\n"
               "\r\n"
               "USEFUL FILES (in the game folder)\r\n"
               "  loader.log                  CrabeLoader's log. Attach it to any bug report.\r\n"
               "  Crabe\\crabe.toml            Settings: keys, profiles, updates.\r\n"
               "  Crabe\\crash-*.txt, *.dmp    Crash reports (only created if the game crashes).\r\n"
               "\r\n"
               "UPDATES\r\n"
               "  At launch, CrabeLoader asks GitHub whether a newer version exists and\r\n"
               "  asks you in a window. To never do that, add this to Crabe\\crabe.toml:\r\n"
               "      [updates]\r\n"
               "      check = false\r\n"
               "\r\n"
               "UNINSTALL\r\n"
               "  Delete bink2w32.dll, then rename bink2w32_orig.dll to bink2w32.dll.\r\n"
               "  (Or, in Steam: Verify integrity of game files.)\r\n"
               "\r\n"
               "\r\n"
               "CrabeLoader - LISEZMOI\r\n"
               "======================\r\n"
               "\r\n"
               "CrabeLoader est install\xC3\xA9. Ce dossier \"mods\" est pr\xC3\xAAt \xC3\xA0 recevoir vos mods.\r\n"
               "\r\n"
               "AJOUTER UN MOD\r\n"
               "  Placez le dossier du mod ici, avec son fichier mod.json :\r\n"
               "      mods\\nom_du_mod\\mod.json\r\n"
               "  Lancez ensuite le jeu. Aucun autre r\xC3\xA9glage n'est n\xC3\xA9" "cessaire.\r\n"
               "\r\n"
               "TOUCHES PAR D\xC3\x89" "FAUT\r\n"
               "  Insert    Ouvre ou ferme la console de d\xC3\xA9veloppement (overlay).\r\n"
               "  F4        Recharge tous les mods sans red\xC3\xA9marrer le jeu.\r\n"
               "  Les deux se changent dans Crabe\\crabe.toml, section [keybinds] (hotReload, devOverlay).\r\n"
               "\r\n"
               "FICHIERS UTILES (dans le dossier du jeu)\r\n"
               "  loader.log                  Journal de CrabeLoader. \xC3\x80 joindre \xC3\xA0 tout signalement de probl\xC3\xA8me.\r\n"
               "  Crabe\\crabe.toml            Configuration : touches, profils, mises \xC3\xA0 jour.\r\n"
               "  Crabe\\crash-*.txt, *.dmp    Rapports de plantage (cr\xC3\xA9\xC3\xA9s seulement si le jeu plante).\r\n"
               "\r\n"
               "MISES \xC3\x80 JOUR\r\n"
               "  Au lancement, CrabeLoader demande \xC3\xA0 GitHub si une nouvelle version existe\r\n"
               "  et vous pose la question dans une fen\xC3\xAAtre. Pour ne jamais le faire, ajoutez ceci\r\n"
               "  \xC3\xA0 Crabe\\crabe.toml :\r\n"
               "      [updates]\r\n"
               "      check = false\r\n"
               "\r\n"
               "D\xC3\x89SINSTALLER\r\n"
               "  Supprimez bink2w32.dll puis renommez bink2w32_orig.dll en bink2w32.dll.\r\n"
               "  (Ou, dans Steam : V\xC3\xA9rifier l'int\xC3\xA9grit\xC3\xA9 des fichiers du jeu.)\r\n";
    }

} // namespace crabe::installer
