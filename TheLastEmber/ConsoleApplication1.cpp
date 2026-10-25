#include <iostream>
#include <random>
#include <curses.h>                                                                                                                         // fennetre et couleurs
#include <cstdlib>                                                                                                                          // Pour utiliser abs().
#include <fstream>                                                                                                                          // Pour lire les fichiers de niveaux.
#include <string>                                                                                                                           // Pour utiliser des string
#include <vector>                                                                                                                           // Pour stocker la carte dans un tableau dynamique.
#include <windows.h>                                                                                                                        // Nécessaire pour Windows API, musique
#include <mmsystem.h>                                                                                                                       // Nécessaire pour le système multimédia, musique
#pragma comment(lib, "winmm.lib")                                                                                                            // Bibliotheque utilisee pour PlaySound.
using namespace std;                                                                                                                        // simplifie l'écriture pas besoint de std:: partout



int Largeur = 0;
int Hauteur = 0;
int spawnX = 0;                                                                                                                             // Position X du spawn du heros.
int spawnY = 0;                                                                                                                             // Position Y du spawn du heros.
int sortieX = -1;                                                                                                                           // Position X de la sortie.
int sortieY = -1;                                                                                                                           // Position Y de la sortie.
int niveau = 0;
bool gameOver = false;                                                                                                                      // boucle pour que le jeux continue
bool Etat = false;                                                                                                                          // pour lancer le sort
int NB_ENNEMIS = 1;                                                                                                                         // Nombre d'ennemis pour le moment const plutart variables
int niveauMax = 4;                                                                                                                          // Nombre de niveau max pour le moment const plutart variables
bool sonEnnemies = false;
bool sonCacher = false;
bool heroMortEnnemi = false;
bool heroMortHoxygene = false;
bool EnergieInsufisante = false;
bool EnnemieTuable = false;



enum tile_type {                                                                                                                            // diférent terrain
    ground = 0,
    river = 1,
    mountain = 2
};

vector<tile_type> map;                                                                                                                      // Tableau a UNE dimension contenant les terrains de la carte et dynamique grace aux vecteurs.


struct HeroStat
{
    int x;
    int y;
    bool hidden;
    int pv;
    int oxygene;
    int energie;
};


struct enemiesStat
{
    int x;
    int y;
    int pv;
};

struct pointeurEnnemis
{
    int x;
    int y;
};

struct CurseurPosition {
    int x;
    int y;
};


int indice(int x, int y)                                                                                                                        // Transforme les coordonnees (x, y) en indice du tableau
{
    return y * Largeur + x;
}

bool dansCarte(int x, int y)                                                                                                                     // Verifie si une position est dans les limites de la carte.
{
    return x >= 0 && x < Largeur && y >= 0 && y < Hauteur;
}


void Menu()                                                                                                                                     // Affiche le menu principal
{
    initscr();                                                                                                                                  // Initialise PDCurses.
    cbreak();                                                                                                                                   // Lit les touches sans attendre Entree.
    noecho();                                                                                                                                   // affiche pas zqsd
    keypad(stdscr, TRUE);                                                                                                                       // pour les fléches.
    curs_set(0);                                                                                                                                // Masque le curseur, sert a pas grand chose mais c'est mieux pour l'esthétique

    string line;
    ifstream fichierMenu("map/Menu.txt");                                                                                                       // Ouvre le fichier

    while (getline(fichierMenu, line))
    {
        printw("%s\n", line.c_str());                                                                                                           // Affiche chaque ligne du menu. et transforme le string en char* pour l'affichage
    }
    fichierMenu.close();
    PlaySound(TEXT("C:\\Windows\\Media\\Ring09.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);                                               // musique car pourquoi pas

    refresh();                                                                                                                                  // Met a jour l'affichage du menu.

    bool play = false;
    while (!play)
    {
        int choix = getch();                                                                                                                    // Attend une touche du joueur.

        if (choix == 'a' || choix == 'A')
        {
            play = true;                                                                                                                        // Le joueur choisit de commencer.
        }
        else if (choix == 'r' || choix == 'R' || choix == 27)
        {
            play = true;                                                                                                                        // Le joueur choisit de quitter.
            gameOver = true;
        }
    }

    clear();                                                                                                                                    // Efface le menu avant de commencer la partie.
    refresh();
}


bool chargerCarte(const string& nomFichier, enemiesStat*& ennemis)                                                                                                     // Charge la map
{
    ifstream fichier(nomFichier);                                                                                                               // Ouvre le fichier du niveau.
    if (!fichier.is_open()) return false;
    vector<string> lignes;
    string ligne;

    while (getline(fichier, ligne))
    {
        if (!ligne.empty() && ligne.back() == '\r')                                                                                             // suprime les espaces et les retour a la ligne
            ligne.pop_back();                                                                                                                   // suprime le caractere

        if (!ligne.empty())                                                                                                                     // si la ligne n'est pas vide rajoute une ligne au tableaus dinamique par vecteur
            lignes.push_back(ligne);
    }
    fichier.close();

    // Verifie que la carte contient des lignes et que toutes ont la meme largeur.
    if (lignes.empty() || lignes[0].empty())
        return false;

    int largeurCarte = static_cast<int>(lignes[0].size());
    int nombreEnnemis = 0;
    int nombreDepart = 0;
    int nombreSortie = 0;

    for (const string& lignesCarte : lignes)
    {
        if (static_cast<int>(lignesCarte.size()) != largeurCarte)
            return false;

        for (char symbole : lignesCarte)
        {
            if (symbole == 'Z')
                nombreEnnemis++;
            else if (symbole == 'S')
                nombreDepart++;
            else if (symbole == 'E')
                nombreSortie++;
            else if (symbole != '.' && symbole != '~' && symbole != 'A')
                return false; // Refuse les symboles inconnus avant de modifier la carte actuelle.
        }
    }

    if (nombreDepart != 1 || nombreSortie != 1)
        return false; // Une carte doit avoir exactement un depart et une sortie.

    Hauteur = static_cast<int>(lignes.size());
    Largeur = largeurCarte;

    delete[] ennemis;
    NB_ENNEMIS = nombreEnnemis;
    ennemis = new enemiesStat[NB_ENNEMIS]{};

    map.clear();                                                                                                                                // supprime tous les terrains de la carte précédente

    spawnX = -1;                                                                                                                                // Reinitialise la position de depart.
    spawnY = -1;
    sortieX = -1;                                                                                                                               // Reinitialise la position de sortie.
    sortieY = -1;
    int i = 0;
    for (int y = 0; y < Hauteur; y++)
    {
        for (int x = 0; x < Largeur; x++)
        {
            char symbole = lignes[y][x];                                                                                                        // Lit le symbole de la case actuelle.

            if (symbole == 'S')
            {
                spawnX = x;                                                                                                                     // Enregistre la coordonnee X du depart.
                spawnY = y;                                                                                                                     // Enregistre la coordonnee Y du depart.
                map.push_back(ground);                                                                                                          // Le depart est du sol dans le tableau de terrain.
            }
            else if (symbole == 'E')
            {
                sortieX = x;                                                                                                                    // Enregistre la coordonnee X de la sortie.
                sortieY = y;                                                                                                                    // Enregistre la coordonnee Y de la sortie.
                map.push_back(ground);                                                                                                          // La sortie reste du sol dans le tableau de terrain.
            }
            else if (symbole == '.')
                map.push_back(ground);                                                                                                          // Ajoute une case de sol.
            else if (symbole == '~')
                map.push_back(river);                                                                                                           // Ajoute une case de riviere.
            else if (symbole == 'A')
                map.push_back(mountain);                                                                                                        // Ajoute une case de montagne.
            else if (symbole == 'Z') {
                ennemis[i] = { x, y, 1 };
                i++;
                map.push_back(ground);                                                                                                          // Le depart est du sol dans le tableau de terrain.
            }
        }
    }
    return true;                                                                                                                                // La carte a ete chargee avec succes.
}


bool ennemiPresent(const enemiesStat* ennemis, int x, int y)                                                                                   // Verifie si un ennemi vivant occupe une case.
{
    for (int i = 0; i < NB_ENNEMIS; i++)
    {
        const enemiesStat* ennemi = &ennemis[i];                                                                                                // Reference pour simplifier l'ecriture.
        if (ennemi->x == x && ennemi->y == y && ennemi->pv > 0)
            return true;                                                                                                                        // Un ennemi vivant se trouve sur cette case.
    }
    return false;                                                                                                                               // Aucun ennemi vivant sur cette case.
}
void afficherCarte(const HeroStat& hero, const enemiesStat ennemis[])                                                                           // Affiche le terrain, le heros et les ennemis.
{
    erase();                                                                                                                                    // Efface l'affichage precedent.
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);                                                                                                     // Couleur du heros visible.
    init_pair(2, COLOR_BLUE, COLOR_BLACK);                                                                                                      // Couleur du heros cacher.
    init_pair(3, COLOR_RED, COLOR_BLACK);                                                                                                       // Couleur de l'ennemi.
    init_pair(4, COLOR_GREEN, COLOR_BLACK);                                                                                                     // Couleur de la sortie.
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);                                                                                                    // Couleur du sol.
    init_pair(6, COLOR_CYAN, COLOR_BLACK);                                                                                                      // Couleur de la riviere.
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);                                                                                                   // Couleur de la montagne.

    for (int y = 0; y < Hauteur; y++)
    {
        for (int x = 0; x < Largeur; x++)
        {
            char symbole = '.';
            int i = indice(x, y);                                                                                                               // Calcule les coordonées

            if (hero.x == x && hero.y == y)
            {
                if (hero.hidden)
                    symbole = 'S';                                                                                                              // S = heros cacher
                else
                    symbole = 'H';                                                                                                              // H = heros visible
            }
            else if (ennemiPresent(ennemis, x, y))
                symbole = 'X';                                                                                                                  // X = ennemi
            else if (x == sortieX && y == sortieY)
                symbole = 'E';                                                                                                                  // E = sortie
            else if (map[i] == ground)
                symbole = '.';                                                                                                                  // . = sol.
            else if (map[i] == river)
                symbole = '~';                                                                                                                  // ~ = riviere.
            else if (map[i] == mountain)
                symbole = 'A';                                                                                                                  // A = montagne.

            if (has_colors() == true)
            {
                if (symbole == 'H')
                    attron(COLOR_PAIR(1));                                                                                                      // Couleur du heros visible.
                else if (symbole == 'S')
                    attron(COLOR_PAIR(2));                                                                                                      // Couleur du heros cacher.
                else if (symbole == 'X')
                    attron(COLOR_PAIR(3));                                                                                                      // Couleur de l'ennemi.
                else if (symbole == 'E')
                    attron(COLOR_PAIR(4));                                                                                                      // Couleur de la sortie.
                else if (symbole == '.')
                    attron(COLOR_PAIR(5));                                                                                                      // Couleur du sol.
                else if (symbole == '~')
                    attron(COLOR_PAIR(6));                                                                                                      // Couleur de la riviere.
                else if (symbole == 'A')
                    attron(COLOR_PAIR(7));                                                                                                      // Couleur de la montagne.
            }
            mvaddch(y, x, symbole);                                                                                                             // Affiche symbole aux coordonnees (y, x), apres les couleurs pour les appliquer
        }
    }
    attron(COLOR_PAIR(1));                                                                                                                     // Garde une couleur fixe pour les textes de l'interface.
    mvprintw(Hauteur + 1, 0, "Z : haut S : bas Q : gauche D : droite");
    mvprintw(Hauteur + 2, 0, "R : quitter E : sort      Energie pour lancer le sort requis : 20");
    mvprintw(Hauteur + 3, 0, "PV : %d", hero.pv);                                                                                               // Affiche les points de vie restants.
    mvprintw(Hauteur + 5, 0, "Oxygene : %d", hero.oxygene);

    if (hero.hidden)
        mvprintw(Hauteur + 4, 0, "Le heros est cache dans la riviere.");
    else
        mvprintw(Hauteur + 4, 0, "Le heros est visible.");

    mvprintw(Hauteur + 6, 0, "Niveau : %d", niveau);
    mvprintw(Hauteur + 7, 0, "Objectif : atteindre la sortie E.");
    if (has_colors() == false) {
        mvprintw(Hauteur + 8, 0, "Votre terminal ne supporte pas les couleurs.");
    }
    mvprintw(Hauteur + 9, 0, "Energie Total : %d / 30", hero.energie);
    if (EnergieInsufisante == true) {
        mvprintw(Hauteur + 10, 0, "Energie insuffisante : %d / 20", hero.energie);
    }
    if (EnnemieTuable == true) {
        mvprintw(Hauteur + 10, 0, "Aucun ennemi repere, sort echoue, energie perdue");
    }
    refresh();                                                                                                                                  // Met a jour l'ecran.
}


void colisionDetection(HeroStat& hero, enemiesStat ennemis[])                                                                                   // Detecte les collisions entre le heros et les ennemis.
{
    for (int i = 0; i < NB_ENNEMIS; i++)
    {
        if (hero.x == ennemis[i].x && hero.y == ennemis[i].y && ennemis[i].pv > 0)
        {
            hero.pv -= 1;                                                                                                                       // Le heros perd un point de vie et meurt pour l'instant.
            ennemis[i].pv = 0;                                                                                                                  // Elimine l'ennemi qui a touche le heros au cas ou on rajoute des pv au heros.
            Beep(300, 300);
        }
    }
}


void mouvementHero(HeroStat& hero, int mouvement)
{
    int nouveauX = hero.x;
    int nouveauY = hero.y;

    // Le switch regroupe les touches qui font le meme deplacement.
    switch (mouvement)
    {
    case 'z': case 'Z': case KEY_UP:
        nouveauY--;
        break;
    case 's': case 'S': case KEY_DOWN:
        nouveauY++;
        break;
    case 'q': case 'Q': case KEY_LEFT:
        nouveauX--;
        break;
    case 'd': case 'D': case KEY_RIGHT:
        nouveauX++;
        break;
    case 'e': case 'E':
        Etat = true; // Demande de lancer un sort.
        return;
    default:
        return; // Ignore les autres touches.
    }

    if (!dansCarte(nouveauX, nouveauY))
        return; // Le heros ne peut pas sortir de la carte.

    if (map[indice(nouveauX, nouveauY)] == mountain)
        return; // Le heros ne peut pas traverser les montagnes.

    hero.x = nouveauX;
    hero.y = nouveauY;
    hero.hidden = (map[indice(hero.x, hero.y)] == river);
}


void mouvementEnnemie(enemiesStat ennemis[], const HeroStat& hero)                                                                              // Deplace les ennemis vers le heros s'il est proche ou au hasard.
{
    static random_device rd;
    static mt19937 gen(rd()); // Le generateur est conserve entre les tours.
    uniform_int_distribution<int> direction(0, 3);                                                                                              // 0 droite, 1 gauche, 2 bas, 3 haut.
    bool poursuite = false;
    for (int i = 0; i < NB_ENNEMIS; i++)
    {
        if (ennemis[i].pv <= 0)
            continue;                                                                                                                           // Ignore les ennemis morts sinon il reapparaissent.

        int differenceX = hero.x - ennemis[i].x;
        int differenceY = hero.y - ennemis[i].y;
        int nouveauEnnemiX = ennemis[i].x;
        int nouveauEnnemiY = ennemis[i].y;

        if (!hero.hidden && abs(differenceX) + abs(differenceY) <= 4) {
            poursuite = true;
            if (abs(differenceX) > abs(differenceY)) {
                if (differenceX > 0) {
                    nouveauEnnemiX += 1;
                }
                else {
                    nouveauEnnemiX -= 1;
                }
            }
            else if (differenceY != 0) {
                if (differenceY > 0) {
                    nouveauEnnemiY += 1;
                }
                else {
                    nouveauEnnemiY -= 1;
                }
            }
        }
        else {
            int choix = direction(gen);                                                                                                         // Choisit une direction au hasard.
            if (choix == 0) nouveauEnnemiX++;
            else if (choix == 1) nouveauEnnemiX--;
            else if (choix == 2) nouveauEnnemiY++;
            else nouveauEnnemiY--;
        }

        if (!dansCarte(nouveauEnnemiX, nouveauEnnemiY))
            continue; // Ignore un deplacement hors de la carte.
        if (map[indice(nouveauEnnemiX, nouveauEnnemiY)] == mountain || map[indice(nouveauEnnemiX, nouveauEnnemiY)] == river)                    // Les ennemis ne traversent ni montagne ni riviere.
            continue;
        if (nouveauEnnemiX == sortieX && nouveauEnnemiY == sortieY)                                                                             // L'ennemi ne va pas sur la sortie.
            continue;
        if (ennemiPresent(ennemis, nouveauEnnemiX, nouveauEnnemiY) && !(ennemis[i].x == nouveauEnnemiX && ennemis[i].y == nouveauEnnemiY))      // Deux ennemis ne partagent pas une case.
            continue;

        ennemis[i].x = nouveauEnnemiX;                                                                                                          // Actualise la position X de l'ennemi.
        ennemis[i].y = nouveauEnnemiY;                                                                                                          // Actualise la position Y de l'ennemi.
    }
    if (sonEnnemies == false && poursuite == true) {
        PlaySound(TEXT("C:\\Windows\\Media\\Windows Notify System Generic.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        sonEnnemies = true;
    }
    else if (sonEnnemies == true && poursuite == false) {
        PlaySound(NULL, NULL, 0);
        sonEnnemies = false;
    }
}


bool mouvementCurseur(HeroStat& hero, int touche, pointeurEnnemis* pointeur, int nombreCibles, CurseurPosition& curseur) // Déplace le curseur sur les ennemis adjacents.
{
    int curseurX = curseur.x;
    int curseurY = curseur.y;
    mvaddch(curseurY, curseurX, '^');

    if (touche == 27)
        return true;

    if (touche == 'z' || touche == 'Z' || touche == KEY_UP) {
        curseurY--;
    }
    else if (touche == 's' || touche == 'S' || touche == KEY_DOWN) {
        curseurY++;
    }
    else if (touche == 'q' || touche == 'Q' || touche == KEY_LEFT) {
        curseurX--;
    }
    else if (touche == 'd' || touche == 'D' || touche == KEY_RIGHT) {
        curseurX++;
    }

    if (touche == 32) // Espace pour lancer le sort.
    {
        for (int i = 0; i < nombreCibles; i++)
        {
            if (curseurX == pointeur[i].x && curseurY == pointeur[i].y)
            {
                return true;
            }
        }
        return false;
    }

    if (curseurX < hero.x - 1) curseurX = hero.x - 1;
    if (curseurX > hero.x + 1) curseurX = hero.x + 1;
    if (curseurX >= Largeur) curseurX = Largeur - 1;
    if (curseurX < 0) curseurX = 0;

    if (curseurY < hero.y - 1) curseurY = hero.y - 1;
    if (curseurY > hero.y + 1) curseurY = hero.y + 1;
    if (curseurY >= Hauteur) curseurY = Hauteur - 1;
    if (curseurY < 0) curseurY = 0;

    curseur.y = curseurY;
    curseur.x = curseurX;
    return false;
}


bool Spell(HeroStat& hero, enemiesStat* ennemis, pointeurEnnemis*& pointeur, CurseurPosition curseur) // Lance un sort sur un ennemi adjacent.
{
    pointeur = new pointeurEnnemis[9]{};
    int i = 0;

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            if (ennemiPresent(ennemis, hero.x + x - 1, hero.y + y - 1)) {
                pointeur[i].x = hero.x + x - 1;
                pointeur[i].y = hero.y + y - 1;
                i++;
            }
        }
    }

    if (i == 0) {
        EnnemieTuable = true;
        return true; // Le sort a ete tente, mais aucun ennemi n'est a portee.
    }

    sonEnnemies = false;
    PlaySound(TEXT("C:\\Windows\\Media\\Windows Hardware Fail.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

    curseur.x = pointeur[0].x;
    curseur.y = pointeur[0].y;
    bool jouer = false;

    while (!jouer) {
        afficherCarte(hero, ennemis);

        for (int j = 0; j < i; j++) {
            attron(COLOR_PAIR(3));
            mvaddch(pointeur[j].y, pointeur[j].x, '*');
        }

        attron(COLOR_PAIR(1));
        mvaddch(curseur.y, curseur.x, '^');

        int touche = getch();

        if (mouvementCurseur(hero, touche, pointeur, i, curseur) == true) {
            if (touche == 27) {
                PlaySound(NULL, NULL, 0);
                sonEnnemies = false;
                return false; // Echap annule le sort.
            }

            int x = curseur.x;
            int y = curseur.y;

            for (int j = 0; j < NB_ENNEMIS; j++) {
                if (ennemis[j].x == x && ennemis[j].y == y && ennemis[j].pv > 0) {
                    ennemis[j].pv -= 1; // L'ennemi cible perd tous ses PV.
                    break;
                }
            }

            jouer = true;
            PlaySound(NULL, NULL, 0);
            sonEnnemies = false;
            return true;
        }
    }

    return false;
}


void heroMort(HeroStat& hero) // Gere la mort du heros.
{
    if (hero.oxygene <= 0)
    {
        heroMortHoxygene = true; // Le heros est mort par manque d'oxygene.
    }
    else
    {
        heroMortEnnemi = true; // Le heros est mort par un ennemi.
    }
}


bool verifierMort(HeroStat& hero) // Verifie si le heros est mort et termine la partie.
{
    if (hero.pv <= 0)
    {
        heroMort(hero); // Enregistre la cause de la mort.
        gameOver = true;
        return true;
    }
    return false;
}


void heroHoxygene(HeroStat& hero) // Gere l'oxygene du heros.
{
    if (map[indice(hero.x, hero.y)] == river)
    {
        hero.oxygene -= 1; // Le heros est dans la riviere, il perd de l'oxygene.
    }
    else
    {
        if (hero.oxygene < 5) // Le heros regagne de l'oxygene jusqu'a un maximum de 5.
        {
            hero.oxygene += 1;
        }
    }

    if (hero.oxygene <= 0)
    {
        hero.pv -= 1; // Le heros perd des points de vie par manque d'oxygene.
    }
}


int main() // Fonction principale : gere la partie.
{
    string erreurChargement = "";                                                                                                             // Garde le nom du fichier en cas d'erreur.
    Menu();
    PlaySound(NULL, NULL, 0);                                                                                                                  // Arrete la musique du menu.

    while (!gameOver)
    {
        niveau += 1;

        if (niveau > niveauMax)
        {
            gameOver = true;
            break;
        }

        string nomFichier = "map/Niveau" + to_string(niveau) + ".txt";
        enemiesStat* ennemis = nullptr;

        if (!chargerCarte(nomFichier, ennemis)) {
            delete[] ennemis;                                                                                                                   // Libere la memoire meme si le chargement echoue.
            erreurChargement = nomFichier;                                                                                                     // Enregistre le fichier qui pose probleme.
            gameOver = true;
            break;
        }

        HeroStat hero = { spawnX, spawnY, map[indice(spawnX, spawnY)] == river, 1, 5, 4 };

        bool niveauFini = false;

        while (!gameOver && !niveauFini)
        {
            if (hero.energie < 30) {
                hero.energie++;
            }

            afficherCarte(hero, ennemis);
            EnergieInsufisante = false;
            EnnemieTuable = false;

            int touche = getch();

            if (touche == 'r' || touche == 'R' || touche == 27)
            {
                gameOver = true;
                break;
            }

            mouvementHero(hero, touche);

            if (hero.hidden == true && sonCacher == false)
            {
                sonEnnemies = false;
                PlaySound(TEXT("C:\\Windows\\Media\\Ring01.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
                sonCacher = true;
            }
            else if (hero.hidden == false && sonCacher == true)
            {
                PlaySound(NULL, NULL, 0);
                sonCacher = false;
                sonEnnemies = false;
            }

            heroHoxygene(hero);

            if (Etat && hero.energie >= 20)
            {
                pointeurEnnemis* pointeur = nullptr;
                CurseurPosition curseur = { 0, 0 };

                bool sortUtilise = Spell(hero, ennemis, pointeur, curseur);
                Etat = false;

                delete[] pointeur;
                pointeur = nullptr;

                if (sortUtilise)
                    hero.energie -= 20; // L'energie est retiree uniquement si le sort a ete tente.

                sonEnnemies = false;
            }
            else if (Etat) {
                EnergieInsufisante = true;
                Etat = false;
            }

            colisionDetection(hero, ennemis);

            if (verifierMort(hero))
                break;

            if (hero.x == sortieX && hero.y == sortieY)
            {
                niveauFini = true;
                sonEnnemies = false;
                PlaySound(TEXT("C:\\Windows\\Media\\Windows Proximity Notification.wav"), NULL, SND_FILENAME | SND_ASYNC);
                break;
            }

            mouvementEnnemie(ennemis, hero);
            colisionDetection(hero, ennemis);
            verifierMort(hero);
        }

        delete[] ennemis; // Libere la memoire allouee pour les ennemis du niveau.
    }

    PlaySound(NULL, NULL, 0);                                                                                                                  // Arrete tous les sons avant de quitter.
    endwin();                                                                                                                                  // Ferme PDCurses avant les messages de la console.

    if (!erreurChargement.empty())
    {
        std::cout << "\nErreur : impossible de charger le fichier " << erreurChargement << "." << endl;
        std::cout << "Verifie que le fichier existe et que la carte est valide." << endl;
    }

    if (niveau > niveauMax)
    {
        std::cout << "\n\n\n\n                              " << " Felicitation! Vous avez completer tout les niveau" << endl;
    }

    if (heroMortHoxygene == true)
    {
        std::cout << "\n\n\n\n                                        " << " Vouz etes mort noyee" << endl;
    }
    else if (heroMortEnnemi == true) {
        std::cout << "\n\n\n\n                                         " << " Vouz etes morts" << endl;
    }

    std::cout << "\n\n\n\n                                              " << "Game Over\n\n\n" << endl;
    return 0;
}