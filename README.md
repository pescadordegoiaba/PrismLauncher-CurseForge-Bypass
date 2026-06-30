# PrismLauncher-CurseForge-Bypass

Fork baseado em [`Diegiwg/PrismLauncher-Cracked`](https://github.com/Diegiwg/PrismLauncher-Cracked), com foco em:

- modo offline/cracked modular;
- bypass de downloads bloqueados do CurseForge;
- criação de servidor local modded pelo Prism;
- sincronização de mods/configs entre servidor e cliente;
- builds Linux/Windows prontas para release.

Este projeto não é afiliado ao Prism Launcher oficial, Mojang, Microsoft ou CurseForge.

> Aviso: use somente em ambientes offline, LAN, Hamachi ou servidores privados. Não use contas Microsoft/Mojang oficiais neste launcher.

# Operative Lgbt

## Downloads

As builds deste fork ficam em:

https://github.com/pescadordegoiaba/PrismLauncher-CurseForge-Bypass/releases

Assets da release V1:

- `Windows.Release.V1.zip`
- `Pirate.Linux.AppImage.V1.AppImage`
- `Pirate.Linux.Portable.V1.tar.gz`

## Diferenças em Relação ao Diegiwg/PrismLauncher-Cracked

Base comparada: [`Diegiwg/PrismLauncher-Cracked`](https://github.com/Diegiwg/PrismLauncher-Cracked) no commit `0c20d44`.

Versão atual deste fork: commit `bd7576c`.

### 1. Bypass de Downloads CurseForge

O fork adiciona uma camada para resolver downloads do CurseForge quando a API não fornece `downloadUrl`.

Arquivos principais:

- `launcher/modplatform/flame/FileResolvingTask.cpp`
- `launcher/modplatform/flame/FlameInstanceCreationTask.cpp`

O que muda:

- Quando o CurseForge bloqueia o download para launchers terceiros e retorna URL vazia, o launcher monta uma URL direta do CDN `edge.forgecdn.net`.
- O cálculo usa o `fileId` do CurseForge:
  - pasta: `fileId / 1000`
  - arquivo: `fileId % 1000`
  - formato: `https://edge.forgecdn.net/files/{pasta}/{arquivo}/{nome-do-arquivo}`
- Em modo cracked/offline, o fluxo ignora a tela de mods bloqueados e tenta baixar diretamente.

Impacto:

- Modpacks CurseForge com arquivos bloqueados deixam de exigir download manual na maioria dos casos.
- A instalação de modpacks fica mais automática.

Limitação:

- Se o arquivo não existir mais no CDN ou tiver sido removido, o download ainda pode falhar.

### 2. Camada Offline/Cracked Modular

O fork adiciona uma camada própria em `launcher/cracked/`.

Arquivos principais:

- `launcher/cracked/CrackedAccountManager.*`
- `launcher/cracked/CrackedAuthProvider.h`
- `launcher/cracked/CrackedUtils.*`
- `launcher/cracked/OfflineSkinManager.*`
- `launcher/cracked/CMakeLists.txt`

O que muda:

- O modo offline fica isolado em um módulo próprio.
- A flag CMake `ENABLE_CRACKED_MODE` controla a compilação da camada.
- O build define `PRISMLAUNCHER_CRACKED` quando o modo está ativo.
- O launcher pode considerar contas offline válidas sem autenticação Microsoft.
- Há gerenciamento de skins offline persistentes.

Impacto:

- Mais fácil manter a lógica cracked sem espalhar alterações pelo código upstream.
- Mais fácil ligar/desligar o modo cracked em builds customizadas.

### 3. Gerenciador de Servidor Local

Este fork adiciona o botão `Create Server` perto da criação de instância.

Arquivos principais:

- `launcher/ui/dialogs/LocalServerDialog.cpp`
- `launcher/ui/dialogs/LocalServerDialog.h`
- `launcher/ui/MainWindow.cpp`
- `launcher/ui/MainWindow.h`
- `launcher/ui/MainWindow.ui`
- `launcher/CMakeLists.txt`

O gerenciador abre uma janela própria com menu lateral:

- `Overview`
- `Parameters`
- `Mods`
- `Players`
- `Client Sync`
- `Console`

#### 3.1. Overview

Permite configurar:

- pasta do servidor;
- jar de servidor manual, quando necessário;
- memória mínima;
- memória máxima;
- porta;
- aceite do EULA;
- sync de arquivos da instância.

#### 3.2. Instalação Automática de Servidor

O servidor é preparado automaticamente a partir da instância selecionada.

Suporte implementado:

- NeoForge;
- Forge;
- Fabric;
- Quilt;
- Minecraft vanilla/Prism sem loader;
- fallback vanilla para loaders sem instalador dedicado padrão.

Como funciona:

- NeoForge: baixa e executa o installer do servidor.
- Forge: baixa e executa o installer do servidor.
- Fabric: baixa o server launcher pelo Meta Fabric.
- Quilt: baixa o server launcher pelo Meta Quilt.
- Vanilla: consulta o manifest oficial da Mojang e baixa o server jar.

O launcher gera:

- `server.properties`;
- `eula.txt`;
- `start.sh`;
- `start.bat`;
- arquivos de servidor do loader escolhido.

#### 3.3. Servidor em Background

O servidor roda como processo secundário do Prism.

O que muda:

- A janela do gerenciador não bloqueia a janela principal do Prism.
- O usuário pode iniciar o servidor e abrir a instância ao mesmo tempo.
- Fechar a janela enquanto o servidor está rodando apenas esconde a janela.
- O botão `Stop` é usado para desligar o servidor.

#### 3.4. IPs Para Entrar no Servidor

Ao iniciar, o console mostra endereços para conexão:

- `localhost:porta`;
- `127.0.0.1:porta`;
- IPs LAN;
- IPs Hamachi quando detectados em faixa `25.x.x.x`.

Exemplo:

```text
IP FOR JOIN SERVER: localhost:9090
IP FOR JOIN SERVER: 127.0.0.1:9090
IP FOR JOIN SERVER (LAN): 192.168.100.147:9090
IP FOR JOIN SERVER (Hamachi): 25.x.x.x:9090
```

### 4. Server Manager Completo

A aba `Parameters` edita opções comuns do `server.properties`.

Opções disponíveis:

- difficulty;
- default gamemode;
- max players;
- online-mode;
- enforce secure profile;
- command blocks;
- whitelist;
- PvP.

O botão `Apply Server Settings` salva as alterações no `server.properties`.

Quando o servidor está rodando, algumas opções também são aplicadas por comando:

- difficulty;
- default gamemode;
- whitelist on/off.

### 5. Controle de Mods do Servidor

A aba `Mods` mostra os mods da instância e o status no servidor.

Ela informa:

- mod instalado no servidor;
- mod pronto para sincronizar;
- mod ignorado por incompatibilidade com servidor.

Também permite:

- atualizar a lista;
- remover mods selecionados da pasta do servidor.

#### Filtro de Mods Client-Only

O sync do servidor não copia mods claramente client-only.

Exemplos filtrados:

- `voicechat-names`;
- `sodium`;
- `sodium-extra`;
- `iris`;
- `oculus`;
- `optifine`;
- `rubidium`;
- `embeddium`;
- `entityculling`;
- `modmenu`;
- `betterf3`;
- `mouse-tweaks`;
- `zoomify`.

Também são lidos metadados dentro dos `.jar`:

- `fabric.mod.json`;
- `quilt.mod.json`;
- `META-INF/mods.toml`;
- `META-INF/neoforge.mods.toml`.

Isso evita crashes de dedicated server causados por mods que carregam classes de cliente, como:

```text
NoClassDefFoundError: net/minecraft/client/gui/screens/Screen
```

### 6. Controle de Players

A aba `Players` envia comandos administrativos sem precisar digitar tudo no console.

Ações disponíveis:

- whitelist add;
- whitelist remove;
- OP;
- de-OP;
- kick;
- ban;
- pardon;
- gamemode survival;
- gamemode creative.

Também há botões rápidos:

- list players;
- reload whitelist;
- save all.

### 7. Console com Command IDE

A aba `Console` inclui:

- console do servidor;
- filtros de log;
- campo de comando;
- autocomplete;
- presets de comandos Minecraft.

Filtros:

- Info;
- Warnings / errors;
- Debug / trace.

O campo aceita comandos com `/`, como no Minecraft:

```text
/gamerule keepInventory true
/gamemode creative Player
/whitelist add Player
```

Internamente, o launcher remove a `/` antes de enviar ao stdin do servidor, porque o console do dedicated server espera comandos sem barra.

### 8. Client Sync

Este fork adiciona um sistema para facilitar a entrada de jogadores no servidor local.

Limitação importante:

- Minecraft/Forge/NeoForge/Fabric validam mods antes de concluir a entrada no servidor.
- Por isso, o cliente não consegue baixar mods automaticamente depois de já estar conectando sem um mod/plugin próprio no protocolo.

Solução implementada no Prism:

- o servidor gera um pacote `client-sync.zip`;
- o jogador aplica esse pacote na instância antes de abrir o Minecraft.

#### 8.1. Gerar Client Sync no Servidor

No Prism do host:

1. Abra `Create Server`.
2. Vá para `Client Sync`.
3. Clique `Build Client Sync Pack`.
4. Clique `Open Sync Folder`.
5. Envie o `.zip` gerado para o jogador.

O pacote contém:

- `mods`;
- `config`;
- `defaultconfigs`;
- `kubejs`;
- `scripts`;
- `prism-client-sync.json`;
- `README.txt`.

O manifest `prism-client-sync.json` inclui:

- nome da instância;
- versão do Minecraft;
- loader detectado;
- porta do servidor;
- data de geração;
- lista de arquivos;
- tamanho de cada arquivo;
- hash SHA-256 de cada arquivo.

#### 8.2. Usar Client Sync no Cliente

Na instância do jogador:

1. Abra a página `Mods`.
2. Clique `Use Client-Sync`.
3. Selecione o `.zip` gerado pelo servidor.
4. Confirme a extração.
5. Abra o Minecraft e entre no IP do servidor.

O botão valida se o zip contém `prism-client-sync.json` antes de aplicar.

Arquivos principais:

- `launcher/ui/pages/instance/ModFolderPage.cpp`
- `launcher/ui/pages/instance/ModFolderPage.h`
- `launcher/ui/dialogs/LocalServerDialog.cpp`

### 9. Tema Pirate Lava e Recursos Visuais

O fork adiciona um tema e recursos visuais próprios.

Arquivos principais:

- `launcher/ui/themes/PirateLavaTheme.cpp`
- `launcher/ui/themes/PirateLavaTheme.h`
- `launcher/resources/pirate_legacy/*`
- `launcher/ui/themes/ThemeManager.cpp`

Recursos adicionados:

- tema `Pirate Lava`;
- imagens de caveira, navio, espadas e emblema;
- animação de lava;
- estilização da tela CurseForge/Flame.

### 10. Mudanças na Interface Principal

Mudanças na MainWindow:

- botão `Create Server`;
- ação para limpar dados pesados de armazenamento;
- instalação local no Linux via ação de PATH/desktop entry;
- suporte a janelas de servidor modeless por instância.

A limpeza de armazenamento remove dados pesados como:

- instâncias;
- mods centrais;
- skins;
- runtimes Java;
- assets;
- libraries;
- cache;
- metadata;
- logs.

Contas e configurações são preservadas.

### 11. Melhorias em Build

Arquivos adicionados:

- `Compile-Linux.sh`
- `Compile-Windows.sh`
- `msys2-pacman.conf`
- `BUILD_MANJARO.md`

Mudanças importantes:

- build Linux empacotado em `build-linux/package`;
- build Windows 10 em `build-win10/package`;
- zip Windows em `build-win10/PrismLauncher-Win10.zip`;
- script Windows prepara sysroot MSYS2 local;
- `-mguard=cf` só é usado quando o compilador suporta;
- Java helper passou de target/source 7 para 8;
- warnings específicos de Qt/GCC deixaram de quebrar a build.

Comandos:

```bash
./Compile-Linux.sh
./Compile-Windows.sh
```

### 12. Correções de Compatibilidade

Mudanças menores adicionadas:

- ajustes para compilação MinGW/Windows;
- guardas de headers Windows em `FileSystem.cpp`;
- tratamento de mensagens Linux para add-to-PATH/desktop entry;
- suporte a Java mais novo nos subprojetos `launcher` e `javacheck`.

## Resumo em Tabela

| Área | Diegiwg/PrismLauncher-Cracked | PrismLauncher-CurseForge-Bypass |
| --- | --- | --- |
| Offline/cracked | Sim | Sim, modularizado em `launcher/cracked` |
| Skins offline | Básico/variável | Gerenciador dedicado e persistente |
| CurseForge bloqueado | Pode exigir download manual | Tenta URL direta do CDN |
| Criar servidor local | Não | Sim, com manager completo |
| NeoForge/Forge server | Manual | Installer automático |
| Fabric/Quilt server | Manual | Server launcher automático |
| Vanilla server | Manual | Download automático do manifest Mojang |
| Servidor em background | Não | Sim |
| Console com filtros | Não | Sim |
| IDE de comandos | Não | Sim |
| Controle de players | Não | Sim |
| Controle de mods do servidor | Não | Sim |
| Client Sync | Não | Sim, gera e aplica zip |
| Botão `Use Client-Sync` | Não | Sim, na página Mods |
| Build Windows script | Não | Sim |
| Build Linux script | Não | Sim |
| Release Windows/Linux | Não padronizado | Assets `Windows Release V1` e `Pirate Linux*` |
| Tema Pirate Lava | Não | Sim |

## Como Compilar

### Linux

```bash
./Compile-Linux.sh
```

Saída esperada:

```text
build-linux/package/bin/prismlauncher
```

### Windows 10 a Partir do Linux

```bash
./Compile-Windows.sh
```

Saídas esperadas:

```text
build-win10/package/prismlauncher.exe
build-win10/PrismLauncher-Win10.zip
```

## Como Usar o Servidor Local

1. Selecione uma instância Minecraft.
2. Clique `Create Server`.
3. Ajuste porta, memória e parâmetros.
4. Clique `Create / Sync`.
5. Aguarde o servidor iniciar.
6. Use o IP mostrado no console.

Para LAN local:

```text
localhost:porta
127.0.0.1:porta
```

Para outro jogador na mesma rede ou Hamachi:

```text
IP_LAN:porta
IP_HAMACHI:porta
```

## Como Usar Client Sync

No computador do host:

1. `Create Server`
2. `Client Sync`
3. `Build Client Sync Pack`
4. Envie o `.zip` gerado.

No computador do jogador:

1. Abra a instância.
2. Vá em `Mods`.
3. Clique `Use Client-Sync`.
4. Selecione o `.zip`.
5. Abra o jogo.
6. Entre no servidor.

## Arquivos de Release

Os assets seguem a convenção pedida:

- Windows: `Windows Release V1`
- Linux: prefixo `Pirate Linux*`

Na release do GitHub, espaços podem aparecer convertidos em pontos no nome final do asset.

## Licença

Este fork mantém a licença GPLv3 herdada do Prism Launcher.

O projeto não é afiliado ao Prism Launcher oficial.
