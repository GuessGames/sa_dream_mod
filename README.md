# SA Dream Mod

**Кооператив для GTA: San Andreas** — проходьте світ і місії разом з друзями.
**Co-op for GTA: San Andreas** — play the world and the story together with friends.

SA Dream Mod — моя збірка на основі [CoopAndreas](https://github.com/Tornamic/CoopAndreas). Головна мета — стабільна синхронізація: люди, машини й гроші на землі мають бути однаковими в усіх гравців.

SA Dream Mod is my build based on [CoopAndreas](https://github.com/Tornamic/CoopAndreas), focused on stable synchronization: peds, vehicles and money pickups should be the same for every player.

---

## ⬇️ [Завантажити лаунчер / Download the launcher](https://github.com/GuessGames/sa_dream_mod_coop/raw/main/SADreamLauncher.exe)

---

## 🎮 Як грати / How to play

1. Натисніть посилання **«Завантажити лаунчер»** вище — завантажиться `SADreamLauncher.exe`. Якщо Windows покаже «Windows захистив ваш ПК», натисніть **«Докладніше» → «Однаково запустити»** (лаунчер не підписаний).
2. Запустіть лаунчер. Тека гри знайдеться сама, якщо ні — натисніть **«Змінити…»**.
3. Натисніть **«Встановити»**. Лаунчер завантажить мод і зробить резервну копію оригінальних файлів гри.
4. Введіть **нік**, **IP сервера** (його дає той, хто хостить) і **ключ бета-тесту** (кнопка **«Як отримати?»**).
5. Натисніть **«ГРАТИ»**.

Оновлення — та сама кнопка в лаунчері. Лаунчер оновлює і мод, і себе.

> Потрібна гра **GTA San Andreas**. Мод працює на версії `gta_sa.exe` **1.0 US**. Якщо у вас інша версія (Steam, Rockstar Launcher), лаунчер попросить вказати архів або теку з `gta_sa.exe` 1.0 US — ми його не поширюємо.

**English:** download the launcher with the link above (if SmartScreen warns, choose *More info → Run anyway*), run it, press **Install**, enter your nickname, the server IP and the beta key, press **PLAY**. The game must be GTA San Andreas with `gta_sa.exe` **1.0 US** (the launcher asks for it if your version differs).

### 🌐 Гра через інтернет / Playing over the internet
Сервер (`server.exe`, порт **6767 UDP**) запускає один із гравців. Іншим потрібна його адреса: або відкрийте порт 6767 UDP на роутері, або використайте віртуальну мережу на кшталт Radmin VPN / ZeroTier.

---

## ✨ Що змінено / What's different from CoopAndreas

- Ambient-масовку (перехожих, трафік, припарковані машини) генерує **тільки хост**, решта бачить ті самі сутності.
- Люди й машини **не зникають** перед гравцем: хост тримає те, на що дивиться інший гравець.
- **Пауза й згорнуте вікно** більше не зупиняють світ: гра працює у фоні, а сутності гравця на паузі тимчасово передаються іншому.
- Коли гравець **виходить**, його сутності переходять до іншого, а не зникають.
- **Гроші й зброя** з убитих пішоходів однакові в усіх; підібране зникає в усіх.
- Надійніша мережа: явне призначення власника, ID не використовуються повторно одразу, події не переставляються місцями.
- Лаунчер для гравців (UA/EN) з автооновленням і менеджер для розробки, тестів і логів.

Детальний аналіз проблем — [docs/SYNC_ANALYSIS.md](docs/SYNC_ANALYSIS.md).

---

## 🛠 Для розробників / For developers

Збірка, тест у два вікна на одному ПК, логи й публікація релізів — у [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

---

## ❤️ Подяки / Credits

Цей мод не існував би без **[CoopAndreas](https://github.com/Tornamic/CoopAndreas)** — автор **[Tornamic](https://github.com/Tornamic)** і всі контриб'ютори проєкту. Уся основа кооперативу — їхня робота. Підтримайте оригінальний проєкт і приєднуйтесь до їхнього [Discord](https://discord.gg/Z3ugSgFJMU) (там же видається ключ бета-тесту).

This mod would not exist without **[CoopAndreas](https://github.com/Tornamic/CoopAndreas)** by **[Tornamic](https://github.com/Tornamic)** and all of its contributors — the whole co-op foundation is their work. Please support the original project.

Також використано / Also uses: [plugin-sdk](https://github.com/DK22Pac/plugin-sdk), [ENet](http://enet.bespin.org/), [Dear ImGui](https://github.com/ocornut/imgui), ASI Loader / WindowedMode / WidescreenFix від ThirteenAG та Silent.

Оригінальний README CoopAndreas: [docs/UPSTREAM_README.md](docs/UPSTREAM_README.md).

---

## ⚖️ Ліцензія / License

[GPL-3.0](LICENSE), як і в CoopAndreas. Проєкт неофіційний і не пов'язаний з Rockstar Games чи Take-Two Interactive. Для гри потрібна легальна копія GTA: San Andreas; файли гри не поширюються.

Unofficial modification, not affiliated with Rockstar Games or Take-Two Interactive. A legitimate copy of GTA: San Andreas is required; no game files are distributed.
