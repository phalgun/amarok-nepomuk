/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MetadataConfig.h"

#include "amarokconfig.h"
#include "configdialog/dialogs/ExcludedLabelsDialog.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "statsyncing/Config.h"
#include "statsyncing/Controller.h"

MetadataConfig::MetadataConfig( QWidget *parent )
    : ConfigDialogBase( parent )
{
    connect( this, SIGNAL(changed()), parent, SLOT(updateButtons()) );

    setupUi( this );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Small (200 px)" ), QVariant( 200 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Medium (400 px)" ), QVariant( 400 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Large (800 px)" ), QVariant( 800 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Huge (1600 px)" ), QVariant( 1600 ) );

    m_writeBack->setChecked( AmarokConfig::writeBack() );
    m_writeBack->setVisible( false ); // probably not a usecase
    m_writeBackStatistics->setChecked( AmarokConfig::writeBackStatistics() );
    m_writeBackStatistics->setEnabled( m_writeBack->isChecked() );
    m_writeBackCover->setChecked( AmarokConfig::writeBackCover() );
    m_writeBackCover->setEnabled( m_writeBack->isChecked() );
    if( m_writeBackCoverDimensions->findData( AmarokConfig::writeBackCoverDimensions() ) != -1 )
        m_writeBackCoverDimensions->setCurrentIndex( m_writeBackCoverDimensions->findData( AmarokConfig::writeBackCoverDimensions() ) );
    else
        m_writeBackCoverDimensions->setCurrentIndex( 1 ); // medium
    m_writeBackCoverDimensions->setEnabled( m_writeBackCover->isEnabled() && m_writeBackCover->isChecked() );
    m_useCharsetDetector->setChecked( AmarokConfig::useCharsetDetector() );
    connect( m_writeBack, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_writeBackStatistics, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_writeBackCover, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_writeBackCoverDimensions, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()) );
    connect( m_useCharsetDetector, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );

    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    StatSyncing::Config *config = controller ? controller->config() : 0;
    m_statSyncingConfig = config;
    m_statSyncingProvidersView->setModel( config );
    m_forgetCollectionsButton->setIcon( KIcon( "edit-clear" ) );
    m_synchronizeButton->setIcon( KIcon( "amarok_playcount" ) );
    connect( config, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SIGNAL(changed()) );
    connect( config, SIGNAL(rowsInserted(QModelIndex,int,int)), SIGNAL(changed()) );
    connect( config, SIGNAL(rowsRemoved(QModelIndex,int,int)), SIGNAL(changed()) );
    connect( config, SIGNAL(modelReset()), SIGNAL(changed()) );
    connect( m_forgetCollectionsButton, SIGNAL(clicked(bool)), SLOT(slotForgetCollections()) );
    connect( m_statSyncingProvidersView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             SLOT(slotUpdateForgetButton()) );
    if( controller )
        connect( m_synchronizeButton, SIGNAL(clicked(bool)), controller, SLOT(synchronize()) );
    else
        m_synchronizeButton->setEnabled( false );
    slotUpdateForgetButton();

    qint64 checkedFields = config->checkedFields();
    foreach( qint64 field, StatSyncing::Controller::availableFields() )
    {
        QCheckBox *checkBox;
        if( field == Meta::valLabel ) // special case, we want plural:
        {
            QString name = i18n( "Synchronize Labels" );
            QHBoxLayout *lineLayout = new QHBoxLayout();
            checkBox = new QCheckBox( name );
            QLabel *button = new QLabel();
            button->setObjectName( "configureLabelExceptions" );
            connect( button, SIGNAL(linkActivated(QString)),
                     SLOT(slotConfigureExcludedLabels()) );

            lineLayout->addWidget( checkBox );
            lineLayout->addWidget( button );
            lineLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            m_statSyncingFieldsLayout->addLayout( lineLayout );

            slotUpdateConfigureExcludedLabelsLabel();
        }
        else
        {
            QString name = Meta::i18nForField( field );
            name = i18nc( "%1 is field name such as Play Count", "Synchronize %1", name );
            checkBox = new QCheckBox( name );
            m_statSyncingFieldsLayout->addWidget( checkBox );
        }
        checkBox->setCheckState( ( field & checkedFields ) ? Qt::Checked : Qt::Unchecked );
        checkBox->setProperty( "field", field );
        connect( checkBox, SIGNAL(stateChanged(int)), SIGNAL(changed()) );
    }
}

MetadataConfig::~MetadataConfig()
{
    if( m_statSyncingConfig )
        m_statSyncingConfig.data()->read(); // reset unsaved changes
}

bool
MetadataConfig::isDefault()
{
    return false;
}

bool
MetadataConfig::hasChanged()
{
    // a bit hacky, but updating enabled status here does the trick
    m_writeBackStatistics->setEnabled( m_writeBack->isChecked() );
    m_writeBackCover->setEnabled( m_writeBack->isChecked() );
    m_writeBackCoverDimensions->setEnabled( m_writeBackCover->isEnabled() && m_writeBackCover->isChecked() );

    return
        m_writeBack->isChecked() != AmarokConfig::writeBack() ||
        m_writeBackStatistics->isChecked() != AmarokConfig::writeBackStatistics() ||
        m_writeBackCover->isChecked() != AmarokConfig::writeBackCover() ||
        writeBackCoverDimensions() != AmarokConfig::writeBackCoverDimensions() ||
        m_useCharsetDetector->isChecked() != AmarokConfig::useCharsetDetector() ||
        ( m_statSyncingConfig.data() ? ( checkedFields() != m_statSyncingConfig.data()->checkedFields() ) : false ) ||
        ( m_statSyncingConfig.data() ? m_statSyncingConfig.data()->hasChanged() : false );
}

void
MetadataConfig::updateSettings()
{
    AmarokConfig::setWriteBack( m_writeBack->isChecked() );
    AmarokConfig::setWriteBackStatistics( m_writeBackStatistics->isChecked() );
    AmarokConfig::setWriteBackCover( m_writeBackCover->isChecked() );
    if( writeBackCoverDimensions() > 0 )
        AmarokConfig::setWriteBackCoverDimensions( writeBackCoverDimensions() );
    AmarokConfig::setUseCharsetDetector( m_useCharsetDetector->isChecked() );
    if( m_statSyncingConfig )
    {
        m_statSyncingConfig.data()->setCheckedFields( checkedFields() );
        m_statSyncingConfig.data()->save();
    }
}

void
MetadataConfig::slotForgetCollections()
{
    if( !m_statSyncingConfig )
        return;
    foreach( QModelIndex idx, m_statSyncingProvidersView->selectionModel()->selectedIndexes() )
    {
        QString id = idx.data( StatSyncing::Config::ProviderIdRole ).toString();
        m_statSyncingConfig.data()->forgetProvider( id );
    }
}

void
MetadataConfig::slotUpdateForgetButton()
{
    QItemSelectionModel *selectionModel = m_statSyncingProvidersView->selectionModel();
    // note: hasSelection() and selection() gives false positives!
    m_forgetCollectionsButton->setEnabled( !selectionModel->selectedIndexes().isEmpty() );
}

void
MetadataConfig::slotUpdateConfigureExcludedLabelsLabel()
{
    QLabel *label = findChild<QLabel *>( "configureLabelExceptions" );
    if( !label || !m_statSyncingConfig )
    {
        warning() << __PRETTY_FUNCTION__ << "label or m_statSyncingConfig is null!";
        return;
    }
    int exceptions = m_statSyncingConfig.data()->excludedLabels().count();
    QString begin = "<a href='dummy'>";
    QString end = "</a>";
    label->setText( i18np( "(%2one exception%3)", "(%2%1 exceptions%3)", exceptions,
                           begin, end ) );
}

void
MetadataConfig::slotConfigureExcludedLabels()
{
    ExcludedLabelsDialog dialog( m_statSyncingConfig.data(), this );
    if( dialog.exec() == QDialog::Accepted )
    {
        slotUpdateConfigureExcludedLabelsLabel();
        emit changed();
    }
}

int
MetadataConfig::writeBackCoverDimensions() const
{
    return m_writeBackCoverDimensions->itemData( m_writeBackCoverDimensions->currentIndex() ).toInt();
}

qint64
MetadataConfig::checkedFields() const
{
    qint64 ret = 0;
    foreach( QCheckBox *checkBox, m_statSyncingFieldsLayout->parentWidget()->findChildren<QCheckBox *>() )
    {
        if( checkBox->isChecked() && checkBox->property( "field" ).canConvert<qint64>() )
            ret |= checkBox->property( "field" ).value<qint64>();
    }
    return ret;
}
