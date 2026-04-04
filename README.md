# Compte Rendu — Mini-Shell
## Projet Système — Polytech ET3

**Binôme :** Skali-Serraj  

<img width="688" height="299" alt="Mini-Shell - Projet Polytech ET3" src="https://github.com/user-attachments/assets/7ca6abbf-0d91-465e-a665-a698564ea12f" />

---

## 1. Introduction

Dans ce mini-projet, nous avons implémenté **minishell**, un interpréteur de commandes Unix simplifié en langage C. L'objectif était de comprendre les mécanismes fondamentaux d'un shell : boucle REPL, création de processus, redirections, pipes, gestion des signaux et contrôle des jobs.

---

## 2. Architecture du projet

### Fichiers

| Fichier | Rôle |
|--------|------|
| `main_0.c` | Fichier principal contenant toutes les fonctions |
| `parser.h` | En-tête du parseur |

### Structure générale

Le shell est organisé autour de plusieurs fonctions principales :

- `main()` — boucle REPL principale
- `parse_line()` — découpe la ligne en tokens
- `execute_command()` — aiguilleur : détecte builtins, pipes, redirections
- `execute_pipe()` — gestion des pipes
- `launch_job()` — fork/exec avec gestion des groupes de processus
- `wait_for_job()` — attente d'un job foreground
- `init_shell()` — initialisation du terminal

---

## 3. Partie 1 — Boucle REPL

### Fonctionnalités implémentées

La boucle principale lit une ligne, la parse et l'exécute en boucle infinie.

### Builtins implémentés

- `exit` — quitte le shell proprement
- `echo` — affiche les arguments sur stdout

### Test

```
minishell> echo bonjour le monde
bonjour le monde
minishell> exit
```

---

## 4. Partie 2 — Exécution de commandes

### Fonctionnalités implémentées

Exécution des commandes externes via `fork()` + `execvp()` + `waitpid()`.

### Builtin cd

Le builtin `cd` est implémenté directement dans le shell (sans fork) car `chdir()` doit modifier l'état du processus shell lui-même.

### Tests

```
minishell> ls -l
minishell> pwd
minishell> cd /tmp
minishell> cd
```

---

## 5. Partie 3 — Redirections & Pipes

### Redirections implémentées

| Opérateur | Effet |
|-----------|-------|
| `>` | Redirige stdout vers un fichier (écrase) |
| `>>` | Redirige stdout vers un fichier (ajoute) |
| `<` | Redirige stdin depuis un fichier |

### Pipes

Un pipe connecte la stdout d'une commande à la stdin de la suivante via `pipe()` + `dup2()`.

### Tests

```
minishell> ls -l > /tmp/out.txt
minishell> cat < /tmp/out.txt
minishell> echo hello >> /tmp/out.txt
minishell> ls | grep .c
minishell> cat /etc/hosts | wc -l
```

---

## 6. Partie 4 — Signaux & Background

### Initialisation du shell

Le shell prend le contrôle du terminal au démarrage via `init_shell()` :
- Ignore les signaux `SIGINT`, `SIGQUIT`, `SIGTSTP`, `SIGTTIN`, `SIGTTOU`
- Crée son propre groupe de processus avec `setpgid()`
- Prend le contrôle du terminal avec `tcsetpgrp()`
- Sauvegarde les attributs du terminal avec `tcgetattr()`

### Background avec &

Détection du token `&` en dernier argument → lancement en background sans attendre la fin du processus.

### Tests

```
minishell> sleep 10
^C                        ← sleep tué, shell continue
minishell> sleep 10
^Z
[stopped]                 ← sleep suspendu, shell reprend
minishell> sleep 5 &
[1] 1234                  ← prompt immédiat
```

---

## 7. Fonctionnalités Bonus

### Ouverture de sites web

Le mot clé `google` ouvre Google dans le navigateur par défaut via `system("open ...")`.

```
minishell> google         ← ouvre google.com
minishell> oasisPops      ← ouvre le portail Polytech
```

### Animation Matrix

Le mot clé `matrix` lance une animation de pluie de caractères verts dans le terminal.

```
minishell> matrix
```

### Splash screen au démarrage

Une animation de démarrage avec le nom **SKALI-SERRAJ SHELL** en ASCII art et une barre de chargement s'affiche au lancement du shell.

---

## 8. Ce qui fonctionne / ne fonctionne pas

### Fonctionne ✅

- Boucle REPL complète
- Builtins : `exit`, `echo`, `cd`
- Exécution des commandes externes
- Redirections `>`, `>>`, `<`
- Pipes simples `|`
- Gestion des signaux Ctrl-C et Ctrl-Z
- Lancement en background avec `&`
- Bonus : `google`, `matrix`, splash screen

### Ne fonctionne pas / non implémenté ❌

- Job control complet (partie 5) : `jobs`, `fg`, `bg`
- Pipes multiples enchaînés (`cmd1 | cmd2 | cmd3`)
- Redirections dans les pipes

---

## 9. Conclusion

Ce projet nous a permis de comprendre en profondeur le fonctionnement d'un shell Unix, notamment les mécanismes de `fork/exec`, la gestion des descripteurs de fichiers pour les redirections et pipes, ainsi que le contrôle des processus via les signaux et les groupes de processus.
