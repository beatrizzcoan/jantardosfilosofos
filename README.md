# Jantar dos filósofos

## 1. Objetivos do projeto

Esse projeto implementa o clássico problema do Jantar dos Filósofos, adaptado para execução distribuída em rede.

É implementado um filósofo, que se conecta via socket TCP a um processo coordenador. O coordenador gerencia o uso dos garfos, controla as moedas, aplica penalidades e mantém um placar em tempo real.

O experimento permite observar, de maneira empírica, conceitos fundamentais de sistemas
concorrentes e distribuídos, tais como:

* Exclusão mútua e compartilhamento de recursos.
* Condições de deadlock e inanição.
* Políticas de justiça e estratégias cooperativas.
* Impacto da aleatoriedade e do comportamento individual no desempenho coletivo.

Cada competidor pode ajustar o comportamento do seu filósofo (mais “prudente”, “ganancioso” ou
“reflexivo”) e competir com outros para alcançar o melhor desempenho total.

## 2. Regras do Jogo

* Há N filósofos, numerados de 0 a N − 1, dispostos em círculo.
* Cada filósofo possui dois vizinhos: o da esquerda e o da direita.
* Cada garfo pertence simultaneamente a dois filósofos adjacentes.
* O jogo dura 180 segundos. O coordenador mantém um placar em tempo real.

## 2.1 Estados do Filósofo

* **Pensando:** o filósofo não possui garfos.
* **Esperando:** o filósofo possui apenas um garfo.
* **Comendo:** o filósofo possui os dois garfos.

## 2.2 Regras de comportamento

* O filósofo deve solicitar os garfos ao coordenador antes de comer.
* O ato de comer é permitido apenas quando ambos os garfos estiverem em posse do filósofo.
* Após comer, o filósofo deve liberar ambos os garfos.
* A ausência total de ações por mais de 15 segundos gera penalidade de inatividade, concedendo +1 moeda a cada vizinho.
* Permanecer pensando (sem garfos) por 10 segundos consecutivos concede bônus de reflexão serena: +1 moeda ao próprio filósofo.

O jogo encerra automaticamente ao término do tempo e exibe o ranking final.

## 3. Sistema de Pontuação

| Ação | Efeito em moedas |
| :--- | :--- |
| Comer com sucesso | +2 para o filósofo |
| Reflexão serena (10 s pensando) | +1 para o filósofo |
| Inatividade (15 s sem ação) | +1 para cada vizinho |
| Erro de comando | +1 para cada vizinho |

## 4. Comandos do Protocolo

O filósofo se comunica com o coordenador via socket TCP, utilizando mensagens ASCII finalizadas com \n.
