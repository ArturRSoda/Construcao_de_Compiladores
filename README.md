# Compilador para a Linguagem ConvCC-2025-1

**INE5426 - Construção de Compiladores**  
Implementação de um compilador completo para a linguagem ConvCC-2025-1, incluindo análise léxica, sintática, semântica e geração de código intermediário.

## Funcionalidades Implementadas
| Módulo               | Funcionalidades                                                                 |
|----------------------|---------------------------------------------------------------------------------|
| **Análise Léxica**   | Reconhecimento de tokens, tabela de símbolos, tratamento de erros léxicos       |
| **Análise Sintática**| Gramática LL(1), tabela de análise preditiva, tratamento de erros sintáticos    |
| **Análise Semântica**| Verificação de tipos, escopo, validação de `break`, geração de árvores         |
| **Geração de Código**| SDDs L-atribuídas, código de três endereços                                     |

## Como compilar

Execute o comando abaixo na raiz do projeto:

```sh
make
```

O executável será gerado em `bin/main`.

## Como executar

Utilize o comando para executar o compilador:

```sh
./bin/main <input_code_file>
```

Ex.: `./bin/main input_codes/code_1.txt`

Exemplos de arquivo de entrada estão na pasta `input_codes/`.