# 🔊 Cartão microSD do DFPlayer / DFPlayer microSD card

**PT**

O código toca um som aleatório entre 1 e `TOTAL_SONS` (3) com `player.play(n)`.

1. Formate o microSD em **FAT32**.
2. Coloque os arquivos **na raiz** do cartão, com estes nomes:
   ```
   0001.mp3
   0002.mp3
   0003.mp3
   ```
3. Copie um de cada vez, na ordem. O `play(n)` do DFPlayer segue a ordem em que os arquivos foram gravados no cartão, então copiar fora de ordem pode trocar os sons.
4. Se mudar a quantidade de sons, ajuste `TOTAL_SONS` no código. Se o som mais longo passar de 5 s, ajuste `BLOQUEIO_MS`.

> Os arquivos de áudio não estão no repositório. Use sons que você tenha direito de usar.

**EN**

The code plays a random sound between 1 and `TOTAL_SONS` (3) using `player.play(n)`.

1. Format the microSD as **FAT32**.
2. Put the files **in the root** of the card, named `0001.mp3`, `0002.mp3`, `0003.mp3`.
3. Copy them one at a time, in order: the DFPlayer's `play(n)` follows the order files were written to the card.
4. If you change the number of sounds, update `TOTAL_SONS`. If the longest sound is over 5 s, update `BLOQUEIO_MS`.

> Audio files are not included in this repository. Use sounds you have the right to use.
