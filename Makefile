# =============================================================================
#  VRP Genetic Algorithm — Makefile  (Windows MinGW + Linux/Mac uyumlu)
#
#  Windows : mingw32-make
#  Linux   : make
# =============================================================================

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lm

SRCDIR  = src
INCDIR  = include
OBJDIR  = obj
RESDIR  = results

SRCS = $(SRCDIR)/main.c           \
       $(SRCDIR)/vrp_rng.c        \
       $(SRCDIR)/vrp_instance.c   \
       $(SRCDIR)/vrp_population.c \
       $(SRCDIR)/vrp_crossover.c  \
       $(SRCDIR)/vrp_mutation.c   \
       $(SRCDIR)/vrp_local.c      \
       $(SRCDIR)/vrp_ga.c         \
       $(SRCDIR)/vrp_report.c

OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

ifeq ($(OS),Windows_NT)
    TARGET = vrp_ga.exe
    MKDIR  = if not exist "$(OBJDIR)" mkdir "$(OBJDIR)" & if not exist "$(RESDIR)" mkdir "$(RESDIR)"
    RUN    = $(TARGET)
    RM     = if exist "$(OBJDIR)" rmdir /s /q "$(OBJDIR)" & if exist "$(TARGET)" del /q "$(TARGET)"
else
    TARGET = vrp_ga
    MKDIR  = mkdir -p $(OBJDIR) $(RESDIR)
    RUN    = ./$(TARGET)
    RM     = rm -rf $(OBJDIR) $(TARGET)
endif

# =============================================================================
.PHONY: all clean run run1 run1v dirs help

all: dirs $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

dirs:
	$(MKDIR)

run: all
	$(RUN)

run1: all
	$(RUN) data/vrpnc1.txt

run1v: all
	$(RUN) data/vrpnc1.txt -v

clean:
	$(RM)

help:
	@echo Windows: mingw32-make  /  Linux-Mac: make
	@echo   all      Derle
	@echo   run      Tum 14 problemi calistir
	@echo   run1     Sadece vrpnc1
	@echo   clean    Temizle
