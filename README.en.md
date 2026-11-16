<p align="center">
🇧🇷 <a href="README.md">Português</a> | 🇺🇸 <b>English</b>
</p>

---

<h1 align="center">⚽ Bola na Rede</h1>

<p align="center">
<b>The goal nobody argues about.</b> An <b>automatic goal detection</b> system built with an <b>ESP32</b> and infrared sensors that tells the ball apart from a person, celebrates the goal with a sound and updates a <b>live scoreboard</b> in the browser.
</p>

<p align="center">
  <img src="docs/media/trave.jpg" alt="PVC goal with the sensors mounted" width="30%"/>
  <img src="docs/media/demo-gol.gif" alt="A kick going in and being detected" width="30%"/>
  <img src="docs/media/demo-chutes.gif" alt="A sequence of kicks into the goal" width="30%"/>
</p>

<p align="center">🔊 <a href="docs/media/demo-gol.mp4">Watch the demo with sound</a></p>

---

## 🎯 About the project

- 🏫 Team project for the **Cyber-Physical Systems Connectivity** course, Software Engineering at **PUCPR**, presented on **Nov 12, 2026**
- ❓ **The problem:** in casual matches, a goal depends on what the players saw and the score is kept by hand, which leads to arguments, forgotten goals and counting mistakes
- 💡 **The solution:** when the ball crosses the line, the system confirms the goal on its own, plays a sound and updates a scoreboard anyone can see on their phone
- 🧩 **The real challenge:** not knowing whether something crossed the line, but **whether it was the ball**. A leg, a hand or someone standing in front of the goal also cuts the beams

---

## ⚙️ How it works

The decision comes from **how long the beam is cut**, not from how many beams are cut: the ball crosses a beam in milliseconds, a person blocks it for much longer.

- ⚡ Each beam fires an **interrupt** on the ESP32, which measures the cut with **microsecond** precision
- ⏱️ A cut of up to **100 ms** may be the ball; longer is a person, and the play is discarded
- 🧹 Cuts shorter than **6 ms** are noise and are ignored
- ✅ After a short cut, the ESP32 waits **220 ms**: if no beam is still cut, it's a **goal**
- 🔊 A goal plays one of **3 sounds** on the DFPlayer and locks out new goals for **5 s**
- 📡 The **ultrasonic sensor** on the crossbar calibrates the empty goal on startup and works as an extra check

<p align="center"><img src="docs/media/slide-bola-x-pessoa.png" alt="Cut duration: ball, fast hand and person (slide in Portuguese)" width="85%"/></p>

---

## 🛠️ Technologies and components

<p>
<img src="https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white" alt="ESP32"/>
<img src="https://img.shields.io/badge/Arduino-00878F?style=for-the-badge&logo=arduino&logoColor=white" alt="Arduino"/>
<img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++"/>
<img src="https://img.shields.io/badge/HTML5-E34F26?style=for-the-badge&logo=html5&logoColor=white" alt="HTML5"/>
<img src="https://img.shields.io/badge/JavaScript-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black" alt="JavaScript"/>
</p>

- 🧠 **ESP32 DevKit V1:** measures the cuts, decides ball vs. person and hosts the web scoreboard
- 🔴 **3 RAD100CM infrared beams:** at 11, 27 and 37 cm from the ground (GPIO 32, 33 and 25)
- 📏 **HC-SR04 ultrasonic sensor:** on the crossbar, 46.4 cm from the ground (Trig on GPIO 13, Echo on GPIO 34 through a 1 kΩ + 2 kΩ divider)
- 🎵 **DFPlayer Mini + microSD + speaker:** RX on GPIO 14, through a 1 kΩ resistor
- 🔋 **Power bank:** makes the whole thing portable
- 🥅 **Small PVC goal (60 × 47 cm)** with a net

---

## 🚀 Running it

1. 💻 Arduino IDE with the ESP32 package (**ESP32 Dev Module** board) and the **DFRobotDFPlayerMini** library
2. 🔐 Copy `firmware/BolaNaRede/secrets.example.h` to `secrets.h` and fill in your Wi-Fi (**2.4 GHz**)
3. 💾 On the microSD (FAT32), copy `0001.mp3`, `0002.mp3` and `0003.mp3` to the root, in that order
4. 📶 Turn the Wi-Fi on before the ESP32, upload **with the goal empty** and open the IP shown in the Serial Monitor (115200)

The scoreboard is at `/` and the goal history at `/historico`.

---

## 📊 Results

Tests on **Nov 11, 2026**, with the net on, kicks from 100 cm and the presentation ball:

- ⚽ **20 of 21 kicks counted (95%)**, 100% for medium and hard kicks
- 🚫 **0 false goals** in 12 attempts with a leg, foot, body or still hand
- 🔁 **Edge cases:** two kicks in a row count only the first, a ball bouncing back from the net counts 1 goal and a hand taking the ball out after a goal doesn't create a new one
- 📈 **Progress:** the first round counted 67% of the balls and had false goals from people. Noise on one beam and sensor bounce were fixed with the 6 ms filter and an 800 ms pause, reaching 95%

<p align="center"><img src="docs/media/slide-resultados-chutes.png" alt="Kick results by strength and height (slide in Portuguese)" width="85%"/></p>

<p align="center">📋 <a href="docs/resultados.md">Full test table (in Portuguese)</a></p>

---

## ⚠️ Limitations

- 🐢 A very slow ball (under ~1.5 m/s) cuts the beam for more than 100 ms and is treated as a person
- ✋ A hand passed quickly on purpose cuts like the ball and may count as a goal
- 🔧 The top beam (GPIO 25) was out of order during the tests

---

## 👥 Team

- 👤 Arthur Lopes Pain
- 👤 João Vitor Vicente Franco de Souza
- 👤 João Pedro Muller Spielmann
- 👤 Leonardo Candelorio de Nardi Ferreira
- 👤 Wellington Nery Gonçalves da Costa
