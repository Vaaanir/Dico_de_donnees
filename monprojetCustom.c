#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "table.h"
#define EXIT 0
#define SET 1
#define LOOKUP 2
#define DUMP 3

int **createTable(int nbLin){ //crée le tableau permettant de faire passer les commandes dans les pipes
    int nbCol = 2;
	int **table = (int **)malloc(sizeof(int*)*nbLin);
	int *table2 = (int *)malloc(sizeof(int)*nbCol*nbLin);
	for(int i = 0 ; i < nbLin ; i++){
		table[i] = &table2[i*nbCol];
	}
	return table;
}
 
void freeTable(int **table){//vide la table ?
	free(table[0]);
	free(table);
}

void init_pipes(int taille, int tubeM[2], int** tubes) {//ne marche pas
    tubes = createTable(taille);   // other pipes
    pipe(tubeM);
    for (int i = 0; i < taille; i++)
    {
        pipe(tubes[i]);
    }
}

void node(int** tubes, int* tubeM, int taille, int ind) {
    //faire la fermeture de tout ce qui est inutile
    close(tubeM[0]);
    for (int i = 0; i < taille; i++)
    {
        if(i == ind) {
            close(tubes[i][0]);
        } else if (i == ind-1 || (ind==0 && i == taille-1)) {
            close(tubes[i][1]);
        } else {
            close(tubes[i][0]);
            close(tubes[i][1]);
        }
    }
    int position = ind;
    PTable_entry ptete = (PTable_entry) malloc(sizeof(Table_entry));
    ptete = NULL; 
    int y;
    if (ind == 0)
    {
        position = taille;
    }
    read(tubes[position-1][0], &y, sizeof(int));
    if (y==1)   // La commande est SET
    {
        int key;
        read(tubes[position-1][0], &key, sizeof(int));  //on recup la clé
        char value[128];
        read(tubes[position-1][0], value, sizeof(char)*128);   //on recup la valeur
        //close(tubes[position-1][0]);  //on ferme le tube qu'on a fini de read
        if (key%taille==ind) {  // on est dans noeud qui gère le set
            store(&ptete,key,value);
            fprintf(stdout,"clé : %d \t valeur : %s",key,value); // test pour voir si on as bien la valeur
            // il faut ensuite signaler au controller qu'on a terminé
            //write(tubeM[1], &success, sizeof(int));
        } else {
            // on doit write les 3 infos dans le prochain tube
            write(tubes[ind][1], &y, sizeof(int));
            write(tubes[ind][1], &key, sizeof(int));
            write(tubes[ind][1], &value, sizeof(char)*128);
        }
        //close(tubes[position][0]);  //on ferme le tube qui a (maybe) était write
        //close(tubeM[1]);    //on ferme l'accès au controller
    }
    //si commande est exit
    /*close(tubes[position-1][0]);  //on ferme le tube qu'on a fini de read
    close(tubes[position][1]);  //on ferme le tube qui a (maybe) était write
    close(tubeM[1]);    //on ferme l'accès au controller
    */
    
    
}

void controller(int taille) {
    int ent;
    int cle;
    int nodec = 0;
    char valeur[128];

    int tubeM[2];   // main pipe
    int **tubes = createTable(taille);   // other pipes 
    pipe(tubeM);
    for (int i = 0; i < taille; i++)
    {
        pipe(tubes[i]);
    }
    // init_pipes(taille, tubeM, tubes);
    
    while (nodec < taille)
    {
        switch (fork())
            {
            case -1:
                perror("fork");
                    exit(-1);
                break;
            
            case 0:
                node(tubes, tubeM, taille, nodec);
                printf("pid -> %d\n",getpid());
                exit(0);
                break;

            default://faire if pour vérifier si c'est un processus fils
                do
                {
                    fprintf(stdout,"Merci de saisir la commande (0 = exit, 1 = set, 2 = lookup, 3 = dump) : ");
                    fscanf(stdin,"%d",&ent);
                    switch (ent)
                        {
                        case EXIT:
                            //exit (envoi du signal d'arret à chaque node)
                            printf("j'exécute avant de sortir");
                            exit(0);
                            break;

                        case SET:
                            fprintf(stdout,"Saisir la cle (decimal number) : ");
                            fscanf(stdin,"%d",&cle);
                            fprintf(stdout,"Saisir la valeur (chaine de caracteres, max 128 chars) : ");
                            fscanf(stdin,"%s",valeur);
                            //faire exec set à node 0 qui transmettra au bon node
                            //ecrit dans tube(n-1) 
                            int x = 1;
                            write(tubes[taille-1][1], &x, sizeof(int));
                            write(tubes[taille-1][1], &cle, sizeof(int));
                            write(tubes[taille-1][1], &valeur, sizeof(char)*128);
                            break;

                        case LOOKUP:
                            fprintf(stdout,"Saisir la cle (decimal number) : ");
                            fscanf(stdin,"%d",&cle);
                            //faire exec lookup à node
                            //ecrit dans tube(n-1) la clé
                            break;

                        case DUMP:
                            //dump (debug)
                            break;
                        
                        default:
                            break;
                        }
                } while (ent!=0);
                // int y;
                // read(tubes[taille-1][0], &y, sizeof(int));
                // int key;
                // read(tubes[taille-1][0], &key, sizeof(int));
                // char string[128];
                // read(tubes[taille-1][0], string, sizeof(char)*128);
                // printf("Cmd -> %d Clé -> %d Valeur -> %s\n", y, key, string);
                break;
            }
            nodec++;
    }
    
    
    
}



int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr,"usage : ./monprojet int\n");
        exit(-1);
    }
    int n = atoi(argv[1]);
    controller(n);
    //essayer do while merci de rentrer... et appeler controller dedans
    return 0;
}
