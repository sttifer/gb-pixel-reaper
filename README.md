# Pixel Reaper

Auto battler de sobrevivência para Game Boy (DMG), feito como projeto do dotmatrix.
Uma run vai do título ao game over ou à vitória em 10 minutos, com progressão por
cartas válida só dentro da run.

## Abrir e rodar

O projeto é uma pasta do editor: abra no dotmatrix
e rode. Sem o editor, dá para compilar a ROM direto:

```bash
npx tsx scripts/build-rom.mts ../pixel-reaper/main.c ../pixel-reaper/pixel-reaper.gb
```

(a partir da pasta do dotmatrix; a ROM sai com 32 KB.)

## Controles

| Botão | Ação |
| --- | --- |
| D-pad | move em 8 direções |
| A | confirma (título, carta, pausa, fim de run) |
| B | volta ao menu no fim da run, sai da pausa |
| START | pausa, e começa a run no título |

O ataque é automático: a foice sai sozinha na direção do inimigo mais próximo.

## Arquivos

| Arquivo | O que é |
| --- | --- |
| `main.c` | máquina de estados das seis telas, HUD, relógio da run |
| `arena.c` | herói, horda, lâminas, drops, spawn e desenho |
| `cards.c` | as sete cartas: nome, texto e o que cada uma altera |
| `tiles.txt` | arte 8x8, propositalmente tosca, para você desenhar por cima |
| `map.txt` | a arena, 32x32 tiles, com parede na borda |
| `sfx.txt` | 8 efeitos do jogo e 4 usados só pela música |
| `music.txt` | duas músicas: menu (song 0) e run (song 1) |

Tiles: 0 vazio, 1-3 chão, 4 parede, 5 lâmina, 6 gema de XP, 7 cura, 8-9 herói
(parado e andando), 10-12 inimigos (comum, rápido, tanque), 13 ouro, 14-15 barra
cheia e vazia, 16 cursor, 17 caveira.

## Decisões que valem saber antes de mexer

**O orçamento de frame é sprite.** Medindo no emulador, cada `obj()` custa por volta
de 3300 ciclos: 10 sprites por frame já colocam o fim do `draw()` na linha 93 de 154.
Por isso as pools são pequenas (6 inimigos, 2 lâminas, o herói) e o resto do jogo
cabe no que sobra. Aumentar `ENEMIES` em `arena.c` é a primeira coisa que derruba o
frame rate; medi 6 inimigos ≈ 10% de frames perdidos com a arena cheia, 5 ≈ 6%, e 8
já passa de 20%.

**O que um kill deixa cai no fundo, não em sprite.** Gema, cura e ouro são células do
mapa escritas com `set_tile`, e o tile original volta quando o item é pego. Isso só
funciona porque o nível tem exatamente o tamanho do mapa de hardware (32x32): numa
arena maior, uma célula fora da câmera não é escrita. É também por isso que
`start_run` chama `load_bkg(0)`: o mapa volta como foi desenhado, senão a run
seguinte começa com as gemas da anterior no chão.

**O ímã virou alcance.** Como o drop é uma célula fixa, a carta SOUL PULL aumenta o
raio de coleta em vez de puxar o item; é o mesmo upgrade visto do outro lado e não
custa movimento nenhum.

**Posições são em quartos de pixel.** Um inimigo lento anda 1/4 de pixel por frame em
vez de ficar parado três frames de cada quatro. A divisão por 4 só acontece na hora
de desenhar.

**As telas que não são a run são a janela em (0, 0).** A arena continua carregada
atrás; voltar é uma chamada, não um reload. Sprites só são submetidos no estado de
jogo, então nada aparece por cima dos menus.

## Ajustes rápidos

| Onde | O quê |
| --- | --- |
| `arena.c` `ENEMIES` / `BLADES` | tamanho da horda e das lâminas em voo (custo de frame) |
| `arena.c` `spawning()` | ritmo de spawn e quando cada tipo entra (25 s, 150 s, 300 s) |
| `arena.c` `foe_life` / `foe_speed` / `foe_harm` | stats dos três tipos |
| `main.c` `WIN_SECS` | duração da run (600 s) |
| `cards.c` `cards_take()` | o que cada carta faz, e `card_ready()` os tetos |
| `cards.c` `REROLL_COST` | o pre�o da m�o nova |
| `main.c` `FADE_FRAMES` | quantos frames dura cada um dos quatro passos do fade |

## Fora do MVP

Sem progressão permanente, sem loja, sem múltiplos personagens ou mapas, sem chefes
e sem conquistas, como o GDD pediu. Suporte a GBC também fica para depois: o projeto
está em `color: off` e a arte foi feita para os quatro tons do DMG.
