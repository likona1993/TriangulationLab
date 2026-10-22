#include "DebugControlsWidget.h"
#include "../facade/TriangulationFacade.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

DebugControlsWidget::DebugControlsWidget(QWidget* parent)
    : QWidget(parent)
{
    m_prevButton = new QPushButton(QStringLiteral("<"), this);
    m_nextButton = new QPushButton(QStringLiteral(">"), this);
    m_stepLabel = new QLabel(this);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->addWidget(m_prevButton);
    layout->addWidget(m_stepLabel);
    layout->addWidget(m_nextButton);

    updateState(-1, 0);

    connect(m_prevButton, &QPushButton::clicked, this, [this]() {
        if (m_facade && m_facade->stepBackward()) {
            emit userNavigated();
        }
    });
    connect(m_nextButton, &QPushButton::clicked, this, [this]() {
        if (m_facade && m_facade->stepForward()) {
            emit userNavigated();
        }
    });
}

void DebugControlsWidget::setFacade(facade::TriangulationFacade* facade)
{
    if (m_facade) {
        disconnect(m_facade, nullptr, this, nullptr);
    }

    m_facade = facade;

    if (m_facade) {
        connect(m_facade, &facade::TriangulationFacade::debugStepChanged,
                this, &DebugControlsWidget::updateState);
        updateState(m_facade->getCurrentStep(), m_facade->getTotalSteps());
    }
}

void DebugControlsWidget::updateState(int currentStep, int totalSteps)
{
    const bool hasHistory = totalSteps > 0;

    m_prevButton->setEnabled(hasHistory && currentStep > -1);
    m_nextButton->setEnabled(hasHistory && currentStep + 1 < totalSteps);
    m_stepLabel->setText(hasHistory
        ? tr("Step %1 / %2").arg(currentStep + 1).arg(totalSteps)
        : tr("No history"));
}
