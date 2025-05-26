# Construcao_de_Compiladores

Trabalho realizado durante a matéria Construção de Compiladores - INE5426

## Descrição

Este projeto implementa as etapas iniciais de um compilador, incluindo análise léxica e tabela de símbolos, para uma linguagem fictícia baseada em Python/C. O código fonte está em C++.

## Como compilar

Execute o comando abaixo na raiz do projeto:

```sh
make
```

O executável será gerado em `bin/main`.

## Como executar

Para rodar o analisador léxico com um dos códigos de exemplo:

```sh
./bin/main
```

Por padrão, o arquivo de entrada é `input_codes/input_code_3.txt`. Para testar outros arquivos, altere o caminho no início do arquivo [`src/main.cpp`](src/main.cpp).

## Autores

- Artur (equipe do projeto)

## Observações

- O analisador léxico reconhece identificadores, palavras-chave, números, strings, operadores e pontuações.
- A tabela de símbolos é impressa ao final da execução.
- Consulte os arquivos em `docs/` para detalhes da gramática utilizada.