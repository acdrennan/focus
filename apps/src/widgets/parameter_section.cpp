#include <iostream>

#include <QFont>
#include <QPalette>
#include <QColor>

#include "parameter_section.h"

ParameterSection::ParameterSection(confData* conf, QString sectionTitle, QWidget *parent)
: QWidget(parent) {

    data = conf;

    mainLayout_ = new QVBoxLayout;
    mainLayout_->setSpacing(10);
    mainLayout_->setMargin(10);

    QLabel* title = new QLabel(sectionTitle);
    title->setAlignment(Qt::AlignLeft);

    QFont f = title->font();
    f.setCapitalization(QFont::Capitalize);
    f.setBold(true);
    title->setFont(f);
    QPalette pal = title->palette();
    pal.setColor(QPalette::WindowText, QColor(31, 92, 207));
    title->setPalette(pal);
    mainLayout_->addWidget(title);

    parameterFrame_ = new QFrame(this);
    parameterFrame_->setFrameShadow(QFrame::Plain);
    parameterFrame_->setFrameShape(QFrame::StyledPanel);

    formLayout_ = new QFormLayout();
    formLayout_->setRowWrapPolicy(QFormLayout::WrapLongRows);
    formLayout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout_->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout_->setLabelAlignment(Qt::AlignLeft);
    formLayout_->setVerticalSpacing(0);

    setAutoFillBackground(true);
}

void ParameterSection::addParameter(confElement* element) {
    ParameterInput* inputWidget = new ParameterInput(data, element, this);
    connect(element, SIGNAL(dataChanged()), inputWidget, SLOT(load()));
    inputWidget->setMinimumHeight(18);
    int paramUserLevel = element->userLevel();

    int index = formLayout_->rowCount();
    parameterRowLevelLookup_.insert(element->get("valuelabel"), index);
    parameterUserLevelLookup_.insert(element->get("valuelabel"), paramUserLevel);
    parameterInputLookup_.insert(element->get("valuelabel"), inputWidget);
    
    QLabel* label = new QLabel(element->get("label"));
    label->setToolTip(getWhatsThis(element));
    if (paramUserLevel == 1) {
        QFont f = label->font();
        f.setItalic(true);
        label->setFont(f);
    }
    
    formLayout_->addRow(label, inputWidget);
}

void ParameterSection::loadFromConfig() {
    for (int i = 0; i < parameterInputLookup_.keys().size(); ++i) {
        QString parameterName = parameterInputLookup_.keys()[i];
        parameterInputLookup_[parameterName]->load();
    }
}

void ParameterSection::resetParameters(const QMap<QString, QString>& toBeReset) {
    for (int i = 0; i < parameterInputLookup_.keys().size(); ++i) {
        QString parameterName = parameterInputLookup_.keys()[i];
        if(toBeReset.keys().contains(parameterName)) {
            parameterInputLookup_[parameterName]->saveValue(toBeReset[parameterName]);
            parameterInputLookup_[parameterName]->load();
        }
    }
}

void ParameterSection::finishAddingParameters() {
    parameterFrame_->setLayout(formLayout_);
    mainLayout_->addWidget(parameterFrame_);
    mainLayout_->addSpacing(10);
    setLayout(mainLayout_);
}

void ParameterSection::changeDisplayedParameters(int userLevel, QStringList parametersDisplayed) {
    int shown = parameterInputLookup_.keys().size();
    for (int i = 0; i < parameterInputLookup_.keys().size(); ++i) {
        QString parameterName = parameterInputLookup_.keys()[i];
        int index = parameterRowLevelLookup_[parameterName];
        if (parameterUserLevelLookup_[parameterName] <= userLevel && (parametersDisplayed.contains(parameterName) || parametersDisplayed.contains("*"))) {
            if(formLayout_->itemAt(index, QFormLayout::LabelRole) != nullptr) formLayout_->itemAt(index, QFormLayout::LabelRole)->widget()->show();
            if(formLayout_->itemAt(index, QFormLayout::FieldRole) != nullptr) formLayout_->itemAt(index, QFormLayout::FieldRole)->widget()->show();
            if(formLayout_->itemAt(index, QFormLayout::SpanningRole) != nullptr) formLayout_->itemAt(index, QFormLayout::SpanningRole)->widget()->show();
        } else {
            if(formLayout_->itemAt(index, QFormLayout::LabelRole) != nullptr) formLayout_->itemAt(index, QFormLayout::LabelRole)->widget()->hide();
            if(formLayout_->itemAt(index, QFormLayout::FieldRole) != nullptr) formLayout_->itemAt(index, QFormLayout::FieldRole)->widget()->hide();
            if(formLayout_->itemAt(index, QFormLayout::SpanningRole) != nullptr) formLayout_->itemAt(index, QFormLayout::SpanningRole)->widget()->hide();
            shown --;
        }
    }

    if(shown <= 0) hide();
    else show();
    
    formLayout_->invalidate();
    formLayout_->update();
    update();
    updateGeometry();
}

QString ParameterSection::getWhatsThis(confElement* element) {
    QString whatsthis = element->get("valuelabel") + "<br><br>" + element->get("legend") + "<br><br>";

    confElement::TypeInfo info = element->typeInfo();
    QMap<int, QStringList> widgetRange = info.deduceMinMaxPairs(info.properties);

    if (!widgetRange.isEmpty()) whatsthis += "Range: <br>";

    foreach(int i, widgetRange.keys()) {
        if (widgetRange.size() > 1) {
            whatsthis += QString::number(i + 1) + " : ";
        }
        if (!widgetRange.value(i)[0].isEmpty()) {
            whatsthis += " Min=";
            whatsthis += widgetRange.value(i)[0];
        }
        if (widgetRange.value(i).size() > 1 && !widgetRange.value(i)[1].isEmpty()) {
            whatsthis += " and ";
            whatsthis += " Max=";
            whatsthis += widgetRange.value(i)[1];
            whatsthis += " ";
        }
        whatsthis += "<br>";
    }
    whatsthis += "Example: " + element->get("example") + "<br><br>";
    whatsthis += "<a href=\"" + element->get("help") + "\"> " + element->get("help") + "</a>";

    return whatsthis;

}