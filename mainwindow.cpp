// mainwindow.cpp
#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>
#include <QPainterPath>

// ═══════════════════════════════════════════════════════════════════════════════
//  GameWidget
// ═══════════════════════════════════════════════════════════════════════════════

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(static_cast<int>(GestorNivel::SCENE_W),
                 static_cast<int>(GestorNivel::SCENE_H));
    setFocusPolicy(Qt::StrongFocus);

    try {
        m_nivel = std::make_unique<GestorNivel>();
        m_audio = std::make_unique<AudioEngine>();
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error de inicialización", e.what());
        return;
    }

    m_audio->playMusic(1);

    // 60 fps game loop
    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.setInterval(16); // ~60 fps
    m_timer.start();
    m_clock.start();
}

void GameWidget::switchLevel(int level)
{
    if (!m_nivel) return;
    m_audio->stopMusic();
    m_nivel->loadLevel(level);
    m_audio->playMusic(level);
}

void GameWidget::reset()
{
    if (m_nivel) m_nivel->reset();
}

void GameWidget::tick()
{
    if (!m_nivel) return;

    double dt = m_clock.elapsed() / 1000.0; // seconds
    m_clock.restart();
    dt = qMin(dt, 0.05); // cap at 50 ms to avoid spiral of death

    m_nivel->update(dt);

    // Poll audio events
    if (m_nivel->popBounceEvent()) m_audio->playBounce();
    if (m_nivel->popGlitchEvent()) m_audio->playGlitch();
    if (m_nivel->popScoreEvent())  m_audio->playScore();

    // Propagate score to window title
    if (auto *mw = qobject_cast<MainWindow *>(window())) {
        mw->setWindowTitle(
            QString("Ping Pong   |   Tú: %1   –   IA: %2   |   Nivel %3")
                .arg(m_nivel->scorePlayer())
                .arg(m_nivel->scoreAI())
                .arg(m_nivel->currentLevel())
        );
    }

    update(); // trigger repaint
}

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_nivel)
        m_nivel->draw(&painter);
}

void GameWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat()) return;

    if (e->key() == Qt::Key_Escape) {
        window()->close();
        return;
    }
    if (e->key() == Qt::Key_R) {
        if (m_nivel) m_nivel->reset();
        return;
    }
    if (m_nivel) m_nivel->keyPress(e->key());
}

void GameWidget::keyReleaseEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat()) return;
    if (m_nivel) m_nivel->keyRelease(e->key());
}

// ═══════════════════════════════════════════════════════════════════════════════
//  MainWindow
// ═══════════════════════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Ping Pong — Digital Entities");

    m_game = new GameWidget(this);
    setCentralWidget(m_game);

    // ── Menu bar ──────────────────────────────────────────────────────────────
    QMenu *menuJuego  = menuBar()->addMenu("&Juego");
    QMenu *menuNivel  = menuBar()->addMenu("&Nivel");
    QMenu *menuAyuda  = menuBar()->addMenu("&Ayuda");

    // Juego
    QAction *actReset = menuJuego->addAction("&Reiniciar  (R)");
    menuJuego->addSeparator();
    QAction *actSalir = menuJuego->addAction("&Salir  (Esc)");

    connect(actReset, &QAction::triggered, m_game, &GameWidget::reset);
    connect(actSalir, &QAction::triggered, this, &QWidget::close);

    // Nivel
    QAction *actNivel1 = menuNivel->addAction("Nivel &1 — Fase Estable");
    QAction *actNivel2 = menuNivel->addAction("Nivel &2 — Entorno Inestable");
    QAction *actNivel3 = menuNivel->addAction("Nivel &3 — Singularidad Digital");
    connect(actNivel1, &QAction::triggered, this, [this]{ m_game->switchLevel(1); });
    connect(actNivel2, &QAction::triggered, this, [this]{ m_game->switchLevel(2); });
    connect(actNivel3, &QAction::triggered, this, [this]{ m_game->switchLevel(3); });

    // Ayuda
    QAction *actControles = menuAyuda->addAction("&Controles");
    connect(actControles, &QAction::triggered, this, [this]{
        QMessageBox::information(this, "Controles",
            "Nivel 1:\n"
            "  W / S  → Mover paddle arriba / abajo\n\n"
            "Nivel 2:\n"
            "  W / A / S / D  → Mover paddle en 2 ejes\n\n"
            "Nivel 3:\n"
            "  W / A / S / D  → Mover paddle en 2 ejes (Cortafuegos Móvil)\n\n"
            "R     → Reiniciar\n"
            "Esc   → Salir");
    });

    // Status bar hint
    statusBar()->showMessage("W/S/A/D – mover  |  R – reiniciar  |  Menú Nivel para cambiar nivel");

    // Lock window size precisely to fit all layout items (menu, game widget, statusbar)
    adjustSize();
    setFixedSize(width(), height());
}
