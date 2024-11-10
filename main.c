#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDER 5 

typedef struct {
    char placa[8];      
    char modelo[20];   
    char cor[10];     
    char entrada[6];  
    char saida[6];    
} Registro;

typedef struct BTreeNode {
    int numKeys;                       
    char placas[ORDER - 1][8];        
    long positions[ORDER - 1];        
    struct BTreeNode *children[ORDER];
    int isLeaf;                       
} BTreeNode;

BTreeNode *createNode(int isLeaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    node->isLeaf = isLeaf;
    node->numKeys = 0;
    for (int i = 0; i < ORDER; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void splitChild(BTreeNode *parent, int index, BTreeNode *child) {
    BTreeNode *newNode = createNode(child->isLeaf);
    newNode->numKeys = ORDER / 2 - 1;

    for (int i = 0; i < ORDER / 2 - 1; i++) {
        strcpy(newNode->placas[i], child->placas[i + ORDER / 2]);
        newNode->positions[i] = child->positions[i + ORDER / 2];
    }

    if (!child->isLeaf) {
        for (int i = 0; i < ORDER / 2; i++) {
            newNode->children[i] = child->children[i + ORDER / 2];
        }
    }
    child->numKeys = ORDER / 2 - 1;

    for (int i = parent->numKeys; i >= index + 1; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = newNode;

    for (int i = parent->numKeys - 1; i >= index; i--) {
        strcpy(parent->placas[i + 1], parent->placas[i]);
        parent->positions[i + 1] = parent->positions[i];
    }
    
    strcpy(parent->placas[index], child->placas[ORDER / 2 - 1]);
    parent->positions[index] = child->positions[ORDER / 2 - 1];
    parent->numKeys++;
}

void insertNonFull(BTreeNode *node, char *placa, long pos) {
    int i = node->numKeys - 1;

    if (node->isLeaf) {
        while (i >= 0 && strcmp(placa, node->placas[i]) < 0) {
            strcpy(node->placas[i + 1], node->placas[i]);
            node->positions[i + 1] = node->positions[i];
            i--;
        }
        strcpy(node->placas[i + 1], placa);
        node->positions[i + 1] = pos;
        node->numKeys++;
    } else {
        while (i >= 0 && strcmp(placa, node->placas[i]) < 0) {
            i--;
        }
        i++;
        if (node->children[i]->numKeys == ORDER - 1) {
            splitChild(node, i, node->children[i]);
            if (strcmp(placa, node->placas[i]) > 0) {
                i++;
            }
        }
        insertNonFull(node->children[i], placa, pos);
    }
}

void insert(BTreeNode **root, char *placa, long pos) {
    if ((*root)->numKeys == ORDER - 1) {
        BTreeNode *newRoot = createNode(0);
        newRoot->children[0] = *root;
        splitChild(newRoot, 0, *root);
        int i = 0;
        if (strcmp(placa, newRoot->placas[0]) > 0) {
            i++;
        }
        insertNonFull(newRoot->children[i], placa, pos);
        *root = newRoot;
    } else {
        insertNonFull(*root, placa, pos);
    }
}

void inserirVeiculo(FILE *dataFile, BTreeNode **root, Registro veiculo) {
    fseek(dataFile, 0, SEEK_END);
    long posicao = ftell(dataFile);
    fprintf(dataFile, "%s|%s|%s|%s|%s\n", veiculo.placa, veiculo.modelo, veiculo.cor, veiculo.entrada, veiculo.saida);
    fflush(dataFile);
    insert(root, veiculo.placa, posicao);
}

BTreeNode* buscar(BTreeNode *node, char *placa) {
    int i = 0;
    while (i < node->numKeys && strcmp(placa, node->placas[i]) > 0) {
        i++;
    }
    if (i < node->numKeys && strcmp(placa, node->placas[i]) == 0) {
        return node;
    }
    if (node->isLeaf) {
        return NULL;
    }
    return buscar(node->children[i], placa);
}

void carregarDadosNaArvore(FILE *dataFile, BTreeNode **root) {
    fseek(dataFile, 0, SEEK_SET); 
    char linha[100];
    
    while (fgets(linha, sizeof(linha), dataFile)) {
        Registro veiculo;
        
        if (sscanf(linha, "%7[^|]|%19[^|]|%9[^|]|%5[^|]|%5[^\n]", 
                   veiculo.placa, veiculo.modelo, veiculo.cor, veiculo.entrada, veiculo.saida) == 5) {
            long posicao = ftell(dataFile) - strlen(linha); 
            insert(root, veiculo.placa, posicao);
        } else {
            printf("Erro ao ler linha: %s\n", linha);
        }
    }
}

void exibirVeiculo(FILE *dataFile, BTreeNode *node, int index) {
    fseek(dataFile, node->positions[index], SEEK_SET);
    Registro veiculo;
    fscanf(dataFile, "%7[^|]|%19[^|]|%9[^|]|%5[^|]|%5s", veiculo.placa, veiculo.modelo, veiculo.cor, veiculo.entrada, veiculo.saida);
    
  
    printf("Modelo: %s\n", veiculo.modelo);
    printf("Cor: %s\n", veiculo.cor);
    printf("Entrada: %s\n", veiculo.entrada);
    printf("Saída: %s\n", veiculo.saida);
}

int main() {
    FILE *dataFile = fopen("dados.txt", "a+");
    if (dataFile == NULL) {
        perror("Erro ao abrir o arquivo de dados");
        return 1;
    }

    BTreeNode *root = createNode(1);
    carregarDadosNaArvore(dataFile, &root);

    int opcao;
    char placa[8];
    Registro veiculo;
    while (1) {
        printf("1. Inserir Veículo\n2. Consultar Veículo\n3. Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
                printf("Placa: ");
                scanf("%7s", veiculo.placa);  
                printf("Modelo: ");
                scanf("%19s", veiculo.modelo);  
                printf("Cor: ");
                scanf("%9s", veiculo.cor);  
                printf("Horário de entrada (HH:MM): ");
                scanf("%5s", veiculo.entrada); 
                strcpy(veiculo.saida, ""); 
                inserirVeiculo(dataFile, &root, veiculo);
                break;
            case 2:
                printf("Digite a placa do veículo: ");
                scanf("%7s", placa);  
                BTreeNode *result = buscar(root, placa);
                if (result != NULL) {
                    int index = 0;
                    while (index < result->numKeys && strcmp(placa, result->placas[index]) != 0) {
                        index++;
                    }
                    if (index < result->numKeys) {
                        printf("Veículo encontrado!\n");
                        exibirVeiculo(dataFile, result, index);
                    }
                } else {
                    printf("Veículo não encontrado!\n");
                }
                break;
            case 3:
                fclose(dataFile);
                exit(0);
        }
    }
}
