#pragma once
#include <QWidget>

class QPushButton;
class QLabel;

namespace facade {
class TriangulationFacade;
}

/**
 * @brief Панель пошагового просмотра триангуляции.
 *
 * Кнопки навигации по истории шагов и индикатор текущего шага.
 * Вся навигация и состояние делегируются TriangulationFacade.
 */
class DebugControlsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DebugControlsWidget(QWidget* parent = nullptr);

    void setFacade(facade::TriangulationFacade* facade);

signals:
    // Пользователь нажал кнопку навигации (чтобы холст мог включить debug-режим)
    void userNavigated();

private:
    void updateState(int currentStep, int totalSteps);

    facade::TriangulationFacade* m_facade = nullptr;
    QPushButton* m_prevButton = nullptr;
    QPushButton* m_nextButton = nullptr;
    QLabel* m_stepLabel = nullptr;
};
