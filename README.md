<div align="center">

# 🐱 KuroCat — edição em Português (BR)

**Gatinho de área de trabalho nativo e ultraleve · Reage ao teclado e ao mouse em tempo real (macOS / Windows / Linux / UOS)**

<p>
  <img src="https://img.shields.io/badge/Idioma-PT--BR%20%7C%20EN%20%7C%20中文-2ea44f?style=flat-square" alt="Idiomas"/>
  <img src="https://img.shields.io/badge/Plataformas-macOS%20%7C%20Windows%20%7C%20Linux%20%7C%20UOS-555555?style=flat-square" alt="Plataformas"/>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/tobecwb/KuroCat?style=flat-square&color=8957e5" alt="Licença MIT"/></a>
</p>

<p>
  <b>Português (BR)</b> | <a href="README.zh-CN.md">简体中文</a> | <a href="README.en.md">English</a> ·
  <a href="https://github.com/tobecwb/KuroCat/releases">⬇️ Downloads deste fork</a> ·
  <a href="https://github.com/chanke17/KuroCat">⭐ Projeto original</a>
</p>

<p>
  <img src="assets/demo_typing.gif" width="265" alt="Gato digitando no teclado"/>
  <img src="assets/demo_hunting.gif" width="265" alt="Olhos de caçador ao mexer o mouse rápido"/>
  <img src="assets/demo_combo.gif" width="265" alt="Digitando, arrastando o mouse e apertando a 3ª tecla com a pata traseira"/>
</p>

<sub>macOS nativo (Swift + CoreGraphics + IOHIDManager) · Windows (C++17 + GDI+ + Raw Input) · Linux / UOS (C++17 + GTK3/Cairo + XInput2/evdev)</sub>

</div>

---

> [!NOTE]
> **Este repositório é um fork** de [chanke17/KuroCat](https://github.com/chanke17/KuroCat), criado para disponibilizar o app em **português do Brasil**.
>
> O que muda em relação ao original:
> - Interface completa em **PT-BR** no macOS, Windows e Linux: menus, configurações, provador de skins, alertas, diálogos de atualização e mensagens de validação de skins.
> - **PT-BR é o idioma padrão.** Chinês e inglês continuam disponíveis em **Configurações → Idioma**.
> - **Atualização automática desligada** por segurança (veja [Privacidade](#-privacidade)).
>
> Todo o crédito pelo app vai para o autor original. Este fork segue a mesma [licença MIT](LICENSE).

---

## 💡 Por que o KuroCat?

| Recurso | 🐱 KuroCat | Pets em Electron / Web |
| :--- | :--- | :--- |
| **Tamanho** | **~802 KB** (um único `.exe` sem dependências no Windows) | 80 MB – 150 MB+ (Chromium embutido) |
| **Consumo** | Swift / C++17 nativo, reduz para 30/20/10 fps quando ocioso | Processo de renderização pesado em segundo plano |
| **Em jogos** | **Captura de hardware NKRO real**, funciona em jogos DirectX / Vulkan | Costuma ser bloqueado por IME, foco do jogo ou UAC |
| **Animação 2.5D** | Cabeça esférica estilo Live2D, "olhos de caçador" e gato que estica ao arrastar | Troca de sprites estáticos |
| **Skins** | [Oficina de skins 2.5D](https://workshop.kurocat.cc/) online + [servidor MCP para IA](mcp/README.md) | Edição manual de imagens |
| **Privacidade** | **Processamento 100% local, em memória**, nenhuma tecla é registrada | Código fechado ou cheio de telemetria |

---

## ✨ Destaques

- **🎮 Captura completa de teclado e mouse, para digitar e jogar (NKRO real)**
  - **macOS**: duas camadas, `IOHIDManager` (nível HID do kernel) + `CGEventTap`, sem interferência da troca de método de entrada. Suporta segurar `WASD`, combinações, modificadores sozinhos (`⌘ / ⇧ / ⌥ / ⌃ / Fn`), setas e `F1–F12`.
  - **Windows**: captura de scan codes via `Raw Input API (RIDEV_INPUTSINK)`, que acompanha teclas e micro-movimentos do mouse até dentro de jogos DirectX/Vulkan.
  - **Linux e UOS / Kylin**: duas camadas, `XInput2` + `evdev` do kernel. **Funciona sem `sudo` no UOS / Deepin / Ubuntu X11.** No Wayland, adicione seu usuário ao grupo `input`.
- **🌀 Cabeça esférica 2.5D e olhos de caçador**
  - Cabeça, orelhas, olhos e bigodes giram em perspectiva 2.5D seguindo o cursor. Teclado e mouse ficam de frente para o gato, espelhando a postura natural de mão esquerda no teclado e direita no mouse.
  - Balance o mouse rápido para ativar o **modo caçador**: as pupilas viram bolinhas enormes e brilhantes, o gato se agacha e abana a cauda mais rápido.
- **🐾 Detalhes interativos**
  - **Pata traseira na 3ª tecla e cauda na 4ª**: segure 3 ou 4 teclas ao mesmo tempo e o gato levanta a pata traseira ou enrola a cauda para apertar a tecla extra.
  - **Luz RGB animada no teclado**: um rastro de luz atravessa o teclado na diagonal, mais rápido e mais longo quanto mais teclas você segura (pode ser desligado).
  - **Carinho e soneca**: faça carinho na cabeça e ele fecha os olhos `— —` com partículas brilhantes; deixe parado e ele dorme soltando `z`s.
  - **Ignorar cliques e abrir ao iniciar sessão**: áreas transparentes deixam o clique passar. Também dá para ativar **Ignorar cliques** (para jogos competitivos), **Abrir ao iniciar sessão** e os avisos de atualização.
- **🎨 Skins, temas e estatísticas**
  - Skins nativas (Gatinho preto clássico, Siamês, Pelo curto tigrado prateado, Laranja e branco, Tigrado marrom) e skins da comunidade em `.kurocat` e teclados em `.kurokb`.
  - 3 temas de teclado, mais de 6 cores de olhos, 5 tamanhos, contador de teclas do dia e KPM ao vivo.

<details>
<summary><b>🖼️ Prévia das skins nativas (clique para abrir)</b></summary>
<br/>

| Gatinho preto clássico | Gato siamês |
| :---: | :---: |
| <img src="assets/preview_black.jpg" width="420" alt="Prévia do gatinho preto clássico"/> | <img src="assets/preview_siamese.jpg" width="420" alt="Prévia do gato siamês"/> |

| Pelo curto tigrado prateado | Gato laranja e branco |
| :---: | :---: |
| <img src="assets/preview_shorthair.jpg" width="420" alt="Prévia do pelo curto"/> | <img src="assets/preview_orangeWhite.jpg" width="420" alt="Prévia do gato laranja e branco"/> |

<p align="center">
  <img src="assets/preview_tabby_windows.png" width="520" alt="Gatinho preto e tigrado marrom sentados digitando"/>
</p>

</details>

---

## 📦 Baixar e executar

Os binários desta edição em PT-BR são gerados pelo GitHub Actions deste fork. Eles ficam na aba **[Releases](https://github.com/tobecwb/KuroCat/releases)** (versões marcadas com tag) ou como artefatos na aba **[Actions](https://github.com/tobecwb/KuroCat/actions)**.

### 🍎 macOS (Universal: Apple Silicon e Intel · macOS 11+)
1. Baixe `KuroCat-macOS-universal.zip`, descompacte e arraste `KuroCat.app` para **Aplicativos**. O app fica na barra de menus, sem ícone no Dock.
2. Clique no aviso que aparece embaixo do gato para liberar o `KuroCat` em **Ajustes do Sistema → Privacidade e Segurança → Monitoramento de Entrada**. Depois use **Reiniciar KuroCat** no menu.
3. Com **Ignorar cliques** ativo, use o ícone da barra de menus ou **segure `⌥ Option` para clicar com o botão direito / arrastar**.

<details>
<summary>🛠️ Compilar no macOS</summary>

```bash
chmod +x build.sh
./build.sh
open KuroCat.app
```

</details>

### 🪟 Windows (Windows 7 SP1 / 8 / 10 / 11 x64)
1. Baixe o `KuroCat-Windows-x64.exe` (~**802 KB**, sem dependências externas). Dê dois cliques para abrir; ele fica na bandeja do sistema.
2. Com **Ignorar cliques** ativo, você pode abrir o menu ou sair a qualquer momento:
   - Atalho global: **`Ctrl + Alt + Shift + P`**;
   - Desbloqueio temporário: segure **`Alt`** e clique com o botão direito / arraste o gato;
   - Ícone na bandeja, ou **abra o `KuroCat.exe` de novo** para desativar o Ignorar cliques e abrir o menu.

<details>
<summary>❓ Dúvidas no Windows e como compilar</summary>

- **Segue o mouse mas não o teclado num jogo?** Jogos executados como Administrador isolam as teclas de apps comuns. Clique com o botão direito no gato → **Reiniciar como administrador** (ou ative **Sempre executar como administrador**).
- **O gato some no jogo?** Troque o jogo de *Tela cheia exclusiva* para *Janela sem bordas / Tela cheia em janela*.
- **Continua pedindo atualização depois de baixar?** Um processo antigo ainda estava aberto; as versões novas fecham as antigas ao iniciar.

**Compilar (MSVC / MinGW / CMake):**
```bat
cd windows
build.bat
:: Ou via CMake
cmake -B build && cmake --build build --config Release
```

</details>

### 🐧 Linux e UOS / Kylin (x86_64 e ARM64)
Funciona em **Ubuntu 20.04+ / Debian 11+ / Deepin / UOS / Kylin / Arch / Fedora**:

1. **Instalação sem senha (recomendada no UOS / Kylin)**: extraia `KuroCat-Linux-x86_64.tar.gz` ou `KuroCat-Linux-arm64.tar.gz` e rode `bash install.sh`. Ele instala em `~/.local` sem `sudo`; `bash uninstall.sh` remove.
2. **Pacote `.deb`**:
   ```bash
   sudo apt install ./KuroCat-Linux-amd64.deb
   ```

<details>
<summary>❓ Linux / Wayland / como compilar</summary>

- **Sessões Wayland**: rode `sudo usermod -aG input $USER` uma vez e entre de novo na sessão para ativar a captura via `evdev`.
- **O Kylin bloqueia a instalação do `.deb`?** Coloque *Central de Segurança → Proteção de apps / Controle de execução* em Avisar ou Desligado, ou use o `install.sh`.
- **Compilar (inclusive LoongArch)**:
  ```bash
  sudo apt install g++ pkg-config libgtk-3-dev libcairo2-dev libx11-dev libxi-dev
  cd linux && chmod +x build.sh build_deb.sh
  ./build.sh        # Compila o binário
  ./build_deb.sh    # Gera o instalador .deb
  ```

</details>

---

## 🌐 Ecossistema e ferramentas (do projeto original)

| Recurso | Link | Descrição |
| :--- | :--- | :--- |
| **Site oficial** | [kurocat.cc/en/](https://kurocat.cc/en/) · [中文](https://kurocat.cc/) | Demonstração interativa 2.5D no navegador e perguntas frequentes |
| **Oficina e praça de skins** | [workshop.kurocat.cc](https://workshop.kurocat.cc/) | Editor de skins 2.5D no navegador + skins `.kurocat` e teclados `.kurokb` gratuitos |
| **Guia do OBS** | [kurocat.cc/obs-guide](https://kurocat.cc/obs-guide/) | Captura de janela transparente, tela verde `#00FF00` e configuração para jogos |
| **Servidor MCP do KuroCat** | [mcp/README.md](mcp/README.md) | Servidor MCP em Node.js, sem dependências, para Claude, Cursor e Antigravity criarem e instalarem skins |
| **Formato de skin** | [docs/skin-format.md](docs/skin-format.md) | Referência do esquema `.kurocat` v1 e diagrama de coordenadas |

Esses sites são mantidos pelo autor original e estão em chinês/inglês.

---

## 🤝 Contribuindo

- **Traduções e correções em PT-BR**: abra uma [Issue](https://github.com/tobecwb/KuroCat/issues) ou um Pull Request neste fork.
- **Bugs e recursos do app em geral**: prefira reportar no [projeto original](https://github.com/chanke17/KuroCat/issues), para beneficiar todo mundo.
- **Skins e teclados**: envie um PR para o [KuroCat-Workshop](https://github.com/chanke17/KuroCat-Workshop) para publicar suas criações na praça da comunidade.

<details>
<summary><b>📁 Estrutura do projeto</b></summary>

```text
KuroCat/
├── Sources/                    # Código Swift nativo do macOS (Universal)
├── Resources/                  # Info.plist e AppIcon.icns do macOS
├── build.sh                    # Script de build universal do macOS
├── windows/                    # C++17 nativo do Windows (Win32 + GDI+ + Raw Input)
├── linux/                      # C++17 nativo do Linux e UOS (GTK3 + Cairo + XInput2 + evdev)
├── shared/                     # Código C++ compartilhado (formato de skin, idioma da interface)
├── mcp/                        # Servidor MCP em Node.js, sem dependências
├── docs/                       # Especificação do formato de skin e capturas de tela
└── .github/workflows/build.yml # Build automatizado multiplataforma
```

</details>

---

## 🔒 Privacidade

- Todos os sinais de teclado e mouse são processados **apenas na memória local, em tempo real**. Nenhuma tecla é registrada nem salva.
- **Neste fork a atualização automática está desligada.** O app original baixava e instalava executáveis de um servidor de terceiros sem verificar assinatura nem hash. Aqui, "Verificar atualizações" apenas abre a página de [Releases deste fork](https://github.com/tobecwb/KuroCat/releases) no navegador, e o app não faz requisições de rede por conta própria.
- A assinatura de código via [SignPath.io](https://signpath.io/) citada no projeto original vale só para os binários publicados por `chanke17/KuroCat`. **Os binários deste fork não são assinados.**

## 📄 Licença

Distribuído sob a [licença MIT](LICENSE), a mesma do projeto original.
