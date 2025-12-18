# Memory Tracer

Un allocateur mémoire avec traçage intégré pour visualiser le flow mémoire de vos programmes C en temps réel.

## L'idée

**Voir l'histoire de votre mémoire, pas seulement son état final.**

Memory Tracer fusionne vos logs applicatifs avec le traçage mémoire pour créer une timeline narrative de votre programme :

[ici mettre screenshot]

## Contexte

Développé lors du projet **cub3D** à l'École 42, en travaillant depuis chez moi sur macOS sans accès à Valgrind. J'avais besoin de debugger un parser complexe avec plusieurs phases (parsing → validation → rendering) et je voulais voir en temps réel si chaque phase nettoyait correctement ses ressources.

**L'outil n'a de valeur que dans un contexte narratif** : il s'intègre à vos logs de debugging existants pour raconter l'histoire complète de votre mémoire au fil de l'exécution de votre programme.

## Pourquoi c'est utile

### ID unique par allocation
Suivez le cycle de vie complet : `malloc id: 42` → `free id: 42`. Identifiez instantanément quelle allocation n'est jamais libérée.

### Contexte applicatif intégré
Comprenez *quand* et *pourquoi* une fuite se produit dans le flow de votre programme, pas juste *où* dans le code.

### Validation du error handling
Vérifiez d'un coup d'œil que vos chemins d'erreur nettoient correctement la mémoire.

### Léger et rapide
Overhead minimal, pas de ralentissement.

## Installation & Usage

**1. Copiez les fichiers dans votre projet :**
```bash
cp my_memory.c votre_projet/srcs/
cp my_memory.h votre_projet/include/
```

**2. Modifiez votre Makefile :**
```makefile
# Activez le mode debug
DEBUG = yes

ifeq ($(DEBUG),yes)
    CFLAGS += -DDEBUG_MEMORY
    SRCS += srcs/my_memory.c
endif
```

**3. Incluez le header :**
```c
// Dans votre header principal (ex: votre_projet.h)
#include "my_memory.h"
```

**4. Compilez et lancez :**
```bash
make
./votre_programme
```

Le traçage mémoire s'affichera automatiquement entrelacé avec vos logs !

## Compatibilité

**Actuellement compatible : macOS uniquement**

Support Linux à venir (le parsing des symboles de `backtrace_symbols()` diffère entre les deux systèmes).

---

*Comprenez le flow mémoire de votre programme, pas seulement ses fuites.*