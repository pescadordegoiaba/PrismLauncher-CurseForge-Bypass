# Prism Launcher (Offline / Cracked)

**Fork do Prism Launcher que permite jogar Minecraft completamente offline, sem necessidade de conta Microsoft/Mojang.**

> ⚠️ **AVISO IMPORTANTE**  
> Este launcher **bypassa a autenticação da Microsoft**.  
> - Use **somente** para jogar em modo offline / LAN / servidores privados.  
> - **NUNCA** use contas oficiais da Microsoft/Mojang com este launcher.  
> - Você pode ser banido em servidores oficiais se tentar misturar as coisas.  
> - O uso viola os Termos de Serviço da Mojang/Microsoft. Use por sua conta e risco.

---

## ✨ Melhorias deste fork (modular & de fácil manutenção)

Este não é mais um "fork com gambiarras espalhadas". A partir de 2026 este repositório adota uma arquitetura **modular e testável**:

- Toda a lógica cracked fica isolada em `launcher/cracked/`
- Compila com uma única flag: `-DENABLE_CRACKED_MODE=ON`
- Apenas **3-4 pontos de integração** no código upstream (fácil de manter quando o Prism atualiza)
- Suporte completo a **skins offline persistentes** (maior diferencial vs outros forks)
- Modo offline como padrão + botão bonito "Criar conta Cracked"
- Testes unitários (`ctest -R Cracked`)
- GitHub Action de sincronização automática com o upstream toda semana

---

## 📥 Downloads

Baixe na página de [Releases](https://github.com/Diegiwg/PrismLauncher-Cracked/releases).

Recomendamos **sempre compilar você mesmo** quando possível (veja instruções abaixo). Binários de terceiros podem conter malware.

### Nomes de binários recomendados (nomes neutros)
- `prism-offline`
- `PrismLauncher-Offline.AppImage`
- `org.prismlauncher.Offline` (Flatpak)

---

## 🛠️ Como compilar (Linux / Windows / macOS)

**Manjaro / Arch users**: Veja o guia completo e atualizado em [BUILD_MANJARO.md](BUILD_MANJARO.md). Ele contém os comandos exatos de pacman (sem sudo nos exemplos) e o passo-a-passo de overlay do módulo cracked melhorado.

### Requisitos gerais
- CMake ≥ 3.20
- Qt 6.5+
- Um compilador C++20 decente (GCC 11+, Clang 16+, MSVC 2022)
- Java 17+ (para compilar/testar Minecraft)

### Passo a passo (Linux)

```bash
git clone --recursive https://github.com/Diegiwg/PrismLauncher-Cracked.git
cd PrismLauncher-Cracked

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_CRACKED_MODE=ON \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build build -j$(nproc)
sudo cmake --install build
```

Para **desabilitar** o modo cracked (build idêntico ao upstream):

```bash
cmake -S . -B build -DENABLE_CRACKED_MODE=OFF
```

---

## 🧩 Principais funcionalidades exclusivas

| Funcionalidade                    | Status     | Descrição |
|-----------------------------------|------------|---------|
| Modo offline como padrão          | ✅         | Checkbox em Configurações → Accounts |
| Criar conta cracked com 1 clique  | ✅         | Botão grande + atalho no menu |
| Gerenciador de skins offline      | ✅         | Upload + preview + aplicar + persistência |
| Múltiplas contas offline          | ✅         | Funciona nativamente |
| Integração Modrinth/CurseForge    | ✅         | Baixar mods sem login (publicar requer conta premium) |
| Presets de JVM para PCs fracos    | 🚧         | Planejado (Low-end / Performance) |
| Suporte Java 24+                  | ✅         | Detecção automática |
| Sync automático com upstream      | ✅         | GitHub Action toda semana |

---

## 🧪 Testes

```bash
ctest -R Cracked --output-on-failure
```

---

## 🤝 Contribuindo

1. Leia `docs/cracked-integration-examples.md`
2. Toda nova feature cracked **deve** viver dentro de `launcher/cracked/`
3. Use `#ifdef PRISMLAUNCHER_CRACKED` generosamente
4. Adicione testes
5. Rode `clang-format` e `clang-tidy` nos arquivos modificados

---

## 📜 Licença

Todo o código novo deste fork é GPLv3 (mesmo do Prism Launcher).

O projeto **não é afiliado** ao Prism Launcher oficial.

---

## ❤️ Agradecimentos

- Time original do Prism Launcher / MultiMC
- Comunidade brasileira que mantém vivo o Minecraft offline
- Todos que contribuem com issues, skins bonitas e relatos de bugs

---

**Se este launcher te ajudou, considere dar uma estrela ⭐ e contribuir com código ou documentação!**
