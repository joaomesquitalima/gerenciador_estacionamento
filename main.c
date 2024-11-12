#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDER 5

typedef struct {
    char placa[8];      
    char modelo[20];   
    char cor[10];     
    char entrada[6];  
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

// Função para inserir uma chave no nó não cheio
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

// Função para inserir um novo veículo na árvore-B
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

// Função para salvar a árvore-B em um arquivo binário
void saveBTree(BTreeNode *root, FILE *file) {
    if (root == NULL) {
        return;
    }

    fwrite(&root->numKeys, sizeof(int), 1, file);
    for (int i = 0; i < root->numKeys; i++) {
        fwrite(root->placas[i], sizeof(char), 8, file);
        fwrite(&root->positions[i], sizeof(long), 1, file);
    }

    if (!root->isLeaf) {
        for (int i = 0; i <= root->numKeys; i++) {
            saveBTree(root->children[i], file);
        }
    }
}

// Função para carregar a árvore-B a partir de um arquivo binário
BTreeNode *loadBTree(FILE *file) {
    BTreeNode *root = createNode(1);
    if (root == NULL) {
        return NULL;
    }

    fread(&root->numKeys, sizeof(int), 1, file);
    for (int i = 0; i < root->numKeys; i++) {
        fread(root->placas[i], sizeof(char), 8, file);
        fread(&root->positions[i], sizeof(long), 1, file);
    }

    for (int i = 0; i <= root->numKeys; i++) {
        if (ftell(file) < ftell(file)) {
            root->children[i] = loadBTree(file);
        }
    }
    return root;
}


void displayBTree(BTreeNode *node) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < node->numKeys; i++) {
        printf("%s ", node->placas[i]);
    }
    printf("\n");

    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            displayBTree(node->children[i]);
        }
    }
}
void inserirVeiculo(FILE *dataFile, BTreeNode **root, Registro veiculo) {
    fseek(dataFile, 0, SEEK_END);
    long posicao = ftell(dataFile);
    fprintf(dataFile, "%s|%s|%s|%s\n", veiculo.placa, veiculo.modelo, veiculo.cor, veiculo.entrada);
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
        // Limpeza da linha antes de usar
        linha[strcspn(linha, "\n")] = '\0';  // Remove o '\n' se houver

        Registro veiculo;
        
        // Verifique se a linha tem o formato esperado (4 campos)
        int numCampos = sscanf(linha, "%7[^|]|%19[^|]|%9[^|]|%5[^\n]", 
                                veiculo.placa, veiculo.modelo, veiculo.cor, veiculo.entrada);
        
        if (numCampos == 4) {
            long posicao = ftell(dataFile) - strlen(linha); 
            insert(root, veiculo.placa, posicao);
        } else {
            printf("Erro ao ler linha (esperado 4 campos): %s\n", linha);
        }
    }
}

void exibirVeiculo(FILE *dataFile, BTreeNode *node, int index) {
    // Verificar se a posição no arquivo é válida
    if (node->positions[index] < 0) {
        printf("Posição inválida no arquivo.\n");
        return;
    }

    fseek(dataFile, node->positions[index], SEEK_SET);
    
    Registro veiculo;
    
    // Verificando se a leitura do veículo foi bem-sucedida
    if (fscanf(dataFile, "%7[^|]|%19[^|]|%9[^|]|%5[^\n]", 
               veiculo.placa, veiculo.modelo, veiculo.cor, veiculo.entrada) == 4) {
        printf("Placa: %s\n", veiculo.placa);
        printf("Modelo: %s\n", veiculo.modelo);
        printf("Cor: %s\n", veiculo.cor);
        printf("Entrada: %s\n", veiculo.entrada);
    } else {
        printf("Erro ao ler o registro do veículo. Verifique a formatação dos dados.\n");
    }
}

void imprimirArquivo(FILE *dataFile) {
    char linha[100];
    fseek(dataFile, 0, SEEK_SET);
    while (fgets(linha, sizeof(linha), dataFile)) {
        printf("%s", linha);
    }
}

void listDataInOrder(BTreeNode *node, FILE *dataFile) {
    if (node == NULL) {
        return;
    }

    // Percorrer os filhos à esquerda, em ordem
    for (int i = 0; i < node->numKeys; i++) {
        listDataInOrder(node->children[i], dataFile);
        printf("Placa: %s - Posição: %ld\n", node->placas[i], node->positions[i]);

        // Aqui, podemos acessar os dados no arquivo de dados usando a posição
        fseek(dataFile, node->positions[i], SEEK_SET);
        char linha[100];
        fgets(linha, sizeof(linha), dataFile);
        printf("Dados: %s", linha);  // Exibe os dados associados à placa
    }
    
    // Verificar o último filho (caso não seja folha)
    listDataInOrder(node->children[node->numKeys], dataFile);
}

void modifyData(BTreeNode *root, FILE *dataFile, char *placa, char *newModel, char *newColor, char *newTime) {
    // Procurar pela chave (placa) na árvore-B
    int i = 0;
    while (i < root->numKeys && strcmp(placa, root->placas[i]) > 0) {
        i++;
    }

    // Se a chave for encontrada, modificar o dado no arquivo
    if (i < root->numKeys && strcmp(placa, root->placas[i]) == 0) {
        long position = root->positions[i];
        
        // Modificar os dados no arquivo
        fseek(dataFile, position, SEEK_SET);
        fprintf(dataFile, "%s|%s|%s|%s\n", placa, newModel, newColor, newTime);
        fflush(dataFile); // Garantir que as alterações sejam salvas no arquivo
        printf("Dados modificados para a placa %s.\n", placa);
    } else {
        printf("Placa não encontrada.\n");
    }
}


int main() {
    FILE *dataFile = fopen("dados.txt", "a+");
    if (dataFile == NULL) {
        perror("Erro ao abrir o arquivo de dados");
        return 1;
    }

    BTreeNode *root = createNode(1);
    carregarDadosNaArvore(dataFile, &root);
    imprimirArquivo(dataFile);  

    FILE *indexFile = fopen("indice_btree.bin", "wb");
    if (indexFile == NULL) {
        perror("Erro ao abrir o arquivo de índice");
        return 1;
    }
    
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
                inserirVeiculo(dataFile, &root, veiculo);
                break;
            case 2:
                printf("Digite a placa do veículo a ser consultado: ");
                scanf("%7s", placa);
                BTreeNode *result = buscar(root, placa);
                if (result != NULL) {
                    int index = 0;  
                    while (index < result->numKeys && strcmp(placa, result->placas[index]) > 0) {
                        index++;
                    }
                    exibirVeiculo(dataFile, result, index);
                } else {
                    printf("Veículo não encontrado!\n");
                }
                break;
            case 4:
                 // Carregar a árvore-B a partir do arquivo binário
                indexFile = fopen("indice_btree.bin", "rb");
                if (indexFile == NULL) {
                    perror("Erro ao abrir o arquivo de índice");
                    return 1;
                }
                BTreeNode *loadedRoot = loadBTree(indexFile);
                fclose(indexFile);

                // Exibir a árvore carregada
                printf("Árvore-B carregada:\n");
                displayBTree(loadedRoot);

                fclose(dataFile);
            case 5:
             listDataInOrder(root,dataFile);
            case 3:
                saveBTree(root, indexFile);
                fclose(indexFile);
                fclose(dataFile);
                return 0;
        }
    }

    fclose(dataFile);
    return 0;
}
