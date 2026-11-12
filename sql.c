#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define WIDTH 30
#define HEIGHT 10 

int startx, starty;
char* choix[] =
{ 
	"Afficher tous",
	"Afficher un individu",
	"Ajouter individu",
	"Supprimer individu",
	"Fin",
};
int n_choix = sizeof(choix) / sizeof(char *);
WINDOW* menu_win;
int highlight = 1;
int choice, c, commit, fin = FALSE;
	
uint64_t mysql;
char requete [ 200 ];
char resultat [ 200 ];
char nom [ 31 ], prenom [ 31 ], naissance [ 5 ];
char idnom [ 11 ], idprenom [ 11 ], idnaissance [ 11 ];

void print_menu(WINDOW* menu_win, int highlight);
void effacer_ecran ( int );
void afficher_resultat ( int, char [] );
void extract_integer ( char [], char [] );

extern uint64_t connexion ( void );
extern uint64_t traitement_requete ( uint64_t, char*, char* );
extern void deconnexion ( uint64_t );

int main ( void )
{
	mysql = connexion ( );
	if ( !mysql ) return 1;
	initscr ( );
	clear ( );
	noecho ( );
	cbreak ( );			// Line buffering disabled. pass on everything
	startx = (80 - WIDTH) / 2;
	starty = (24 - HEIGHT) / 2;
	menu_win = newwin ( HEIGHT, WIDTH, starty, startx );
	keypad ( menu_win, TRUE );
	mvprintw ( 0, 0, "Utilise les flèches vers le haut et vers le bas ; appuie sur la touche entrée pour valider" );
	curs_set ( 0 );		// invisible
	refresh ( );
	do
	{
		choice = 0;
		print_menu ( menu_win, highlight );
		while ( TRUE )
		{
			c = wgetch ( menu_win );
			switch(c)
			{	case KEY_UP:
					if(highlight == 1) highlight = n_choix;
					else --highlight;
					break;
				case KEY_DOWN:
					if(highlight == n_choix) highlight = 1;
					else ++highlight;
					break;
				case 0x0A:			// 0x0A = enter key
					choice = highlight;
					break;
				default:
					break;
			}
			print_menu ( menu_win, highlight );
			if ( choice ) break;
		}
		effacer_ecran ( 17 );
		switch ( choice )
		{
			case 1:
				if ( traitement_requete ( mysql, "select lib_nom, lib_prenom, lib_naissance from individu, nom, prenom, naissance where nom=id_nom and prenom=id_prenom and naissance=id_naissance", resultat ) )
					afficher_resultat ( 17, resultat );
				else
				{
					mvprintw ( 17, 0, "Pas de résultat.            " );
					clrtoeol ( );
					refresh ( );
				}
				break;
			case 2:
				curs_set ( 1 );		// visible
				mvprintw ( 17, 10, "Nom : " );
				clrtoeol ( );
				echo ( );
				getstr ( nom );
				noecho ( );
				curs_set ( 0 );		// invisible
				strcpy ( requete, "select lib_nom, lib_prenom, lib_naissance from individu, nom, prenom, naissance where nom=id_nom and prenom=id_prenom and naissance=id_naissance and lib_nom like '" );
				strcat ( requete, nom );
				strcat ( requete, "'" );
				if ( traitement_requete ( mysql, requete, resultat ) ) afficher_resultat ( 18, resultat );
				else
				{
					mvprintw ( 17, 0, "Pas de résultat.              " );
					clrtoeol ( );
					refresh ( );
				}
				break;
			case 3 :
				curs_set ( 1 );		// visible
				mvprintw ( 17, 10, "Nom : " );
				clrtoeol ( );
				echo ( );
				getstr ( nom );
				mvprintw ( 18, 10, "Prénom : ");
				clrtoeol ( );
				getstr ( prenom );
				mvprintw ( 19, 10, "Année de naissance : ");
				clrtoeol ( );
				getstr ( naissance );
				noecho ( );
				curs_set ( 0 );		// invisible
				strcpy ( requete, "select * from individu, nom, prenom, naissance where nom=id_nom and prenom=id_prenom and naissance=id_naissance " );
				strcat ( requete, "and lib_nom like '" );
				strcat ( requete, nom );
				strcat ( requete, "' and lib_prenom like '" );
				strcat ( requete, prenom );
				strcat ( requete, "' and lib_naissance like '" );
				strcat ( requete, naissance );
				strcat ( requete, "'" );
				if ( traitement_requete ( mysql, requete, resultat ) )
				{
					mvprintw ( 17, 0, "L'état civil saisi est déjà dans la base de données." );
					refresh ( );
				}
				else				// insertion
				{
					commit = TRUE;
					strcpy ( requete, "start transaction" );
					traitement_requete ( mysql, requete, resultat );
					strcpy ( requete, "select id_nom from nom where lib_nom like '" );
					strcat ( requete, nom );
					strcat ( requete, "'" );
					if ( traitement_requete ( mysql, requete, resultat ) ) extract_integer ( idnom, resultat );
					else
					{
						strcpy ( requete, "insert into nom (lib_nom) value ('" );
						strcat ( requete, nom );
						strcat ( requete, "')" );
						traitement_requete ( mysql, requete, resultat );
						strcpy ( requete, "select id_nom from nom where lib_nom like '" );
						strcat ( requete, nom );
						strcat ( requete, "'" );
						if ( traitement_requete ( mysql, requete, resultat ) ) extract_integer ( idnom, resultat );
						else commit = FALSE;
					}
					strcpy ( requete, "select id_prenom from prenom where lib_prenom like '" );
					strcat ( requete, prenom );
					strcat ( requete, "'" );
					if ( traitement_requete ( mysql, requete, resultat ) ) extract_integer ( idprenom, resultat );
					else
					{
						strcpy ( requete, "insert into prenom (lib_prenom) value ('" );
						strcat ( requete, prenom );
						strcat ( requete, "')" );
						traitement_requete ( mysql, requete, resultat );
						strcpy ( requete, "select id_prenom from prenom where lib_prenom like '" );
						strcat ( requete, prenom );
						strcat ( requete, "'" );
						if ( traitement_requete ( mysql, requete, resultat ) ) extract_integer ( idprenom, resultat );
						else commit = FALSE;
					}
					strcpy ( requete, "select id_naissance from naissance where lib_naissance like '" );
					strcat ( requete, naissance );
					strcat ( requete, "'" );
					if ( traitement_requete ( mysql, requete, resultat ) ) extract_integer ( idnaissance, resultat );
					else
					{
						strcpy ( requete, "insert into naissance (lib_naissance) value ('" );
						strcat ( requete, naissance );
						strcat ( requete, "')" );
						traitement_requete ( mysql, requete, resultat );
						strcpy ( requete, "select id_naissance from naissance where lib_naissance like '" );
						strcat ( requete, naissance );
						strcat ( requete, "'" );
						if ( traitement_requete ( mysql, requete, resultat ) ) extract_integer ( idnaissance, resultat );
						else commit = FALSE;
					}
					strcpy ( requete, "insert into individu values (" );
					strcat ( requete, idnom );
					strcat ( requete, "," );
					strcat ( requete, idprenom );
					strcat ( requete, "," );
					strcat ( requete, idnaissance );
					strcat ( requete, ")" );
					traitement_requete ( mysql, requete, resultat );
					if ( commit ) strcpy ( requete, "commit" );
					else
					{
						printw ( "********* Saisie annulée. **********" );
						strcpy ( requete, "rollback" );
					}
					traitement_requete ( mysql, requete, resultat );
				}					// fin insertion
				break;
			case 4 :
				curs_set ( 1 );		// visible
				mvprintw ( 17, 10, "Nom : " );
				clrtoeol ( );
				echo ( );
				getstr ( nom );
				mvprintw ( 18, 10, "Prénom : ");
				clrtoeol ( );
				getstr ( prenom );
				mvprintw ( 19, 10, "Année de naissance : ");
				clrtoeol ( );
				getstr ( naissance );
				noecho ( );
				curs_set ( 0 );		// invisible
				strcpy ( requete, "select * from individu, nom, prenom, naissance where nom=id_nom and prenom=id_prenom and naissance=id_naissance " );
				strcat ( requete, "and lib_nom like '" );
				strcat ( requete, nom );
				strcat ( requete, "' and lib_prenom like '" );
				strcat ( requete, prenom );
				strcat ( requete, "' and lib_naissance like '" );
				strcat ( requete, naissance );
				strcat ( requete, "'" );
				if ( !traitement_requete ( mysql, requete, resultat ) )
				{
					mvprintw ( 17, 0, "L'état civil saisi ne figure pas dans la base de données." );
					refresh ( );
				}
				else				// suppression
				{
					strcpy ( requete, "delete from individu where nom in (select id_nom from nom where lib_nom like '" );
					strcat ( requete, nom );
					strcat ( requete, "') and prenom in ( select id_prenom from prenom where lib_prenom like '" );
					strcat ( requete, prenom );
					strcat ( requete, "') and naissance in ( select id_naissance from naissance where lib_naissance like '" );
					strcat ( requete, naissance );
					strcat ( requete, "')" );
					traitement_requete ( mysql, requete, resultat );
				}					// fin suppression
				break;				
			case 5:
				fin = TRUE;
				break;
			default:
				break;
		}
	} while ( !fin );
	endwin();
	deconnexion ( mysql );
	return 0;
}

void effacer_ecran ( int y )
{
	do
	{
		move ( y++, 0 );
		clrtoeol ( );
	} while ( y < 40 );
	refresh ( );
}

void afficher_resultat ( int y, char resultat [ ] )
{
	int p = 0;
	char r;
	
	if ( resultat [ p++ ] == '{' )
	{
		mvprintw ( y, 0, "Nom\t\tPrénom\t\tNaissance" );
		y++;
		do
		{
			r = resultat [ p++ ];
			switch ( r )
			{
				case '[':
					move ( y, 0 );
					y++;
					break;
				case ']':
					printw ( "\t<fin>" );
					clrtoeol ( );
					break;
				case ',':
					printw ( "\t" );
					break;
				default:
					printw ( "%c", r );
			}	
		} while ( resultat [ p ] != '}' );
	}
	else printw ( "Erreur de format du résultat" );
	refresh ( );
}

void extract_integer ( char sortie [], char entree [] )
{
	int e = 3, s = 0;
	char c;
	
	do
	{
	 	c = entree [ e++ ];
		if ( isdigit ( c ) ) sortie [ s++ ] = c; 
	} while ( c != '\'' );
}

void print_menu(WINDOW *menu_win, int highlight)
{
	int x, y, i;	

	x = 2;
	y = 2;
	box(menu_win, 0, 0);
	for(i = 0; i < n_choix; ++i)
	{	if(highlight == i + 1) /* High light the present choice */
		{	wattron(menu_win, A_REVERSE); 
			mvwprintw(menu_win, y, x, "%s", choix[i]);
			wattroff(menu_win, A_REVERSE);
		}
		else mvwprintw(menu_win, y, x, "%s", choix[i]);
		++y;
	}
	wrefresh(menu_win);
}
