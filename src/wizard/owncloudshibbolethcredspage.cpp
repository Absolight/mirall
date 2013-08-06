/*
 * Copyright (C) by Krzesimir Nowak <krzesimir@endocode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "wizard/owncloudshibbolethcredspage.h"
#include "mirall/theme.h"
#include "wizard/owncloudwizardcommon.h"
#include "creds/shibbolethcredentials.h"
#include "creds/shibboleth/shibbolethwebview.h"

namespace Mirall
{

OwncloudShibbolethCredsPage::OwncloudShibbolethCredsPage()
    : AbstractCredentialsWizardPage(),
      _ui(),
      _stage(INITIAL_STEP),
      _browser(0),
      _cookie(),
      _afterInitialSetup(false)
{
    _ui.setupUi(this);

    setTitle(WizardCommon::titleTemplate().arg(tr("Connect to %1").arg(Theme::instance()->appNameGUI())));
    setSubTitle(WizardCommon::subTitleTemplate().arg(tr("Process through Shibboleth form")));

    setupCustomization();
}

void OwncloudShibbolethCredsPage::setupCustomization()
{
    // set defaults for the customize labels.
    _ui.topLabel->hide();
    _ui.bottomLabel->hide();

    Theme *theme = Theme::instance();
    QVariant variant = theme->customMedia( Theme::oCSetupTop );
    if( !variant.isNull() ) {
        WizardCommon::setupCustomMedia( variant, _ui.topLabel );
    }

    variant = theme->customMedia( Theme::oCSetupBottom );
    WizardCommon::setupCustomMedia( variant, _ui.bottomLabel );
}

bool OwncloudShibbolethCredsPage::isComplete() const
{
    return _stage == GOT_COOKIE;
}

void OwncloudShibbolethCredsPage::setVisible(bool visible)
{
    if (!_afterInitialSetup) {
        QWizardPage::setVisible(visible);
        return;
    }

    if (isVisible() == visible) {
        return;
    }
    if (_browser) {
        disposeBrowser(true);
    }
    if (visible) {
        _browser = new ShibbolethWebView(QUrl(field("OCUrl").toString().simplified()));
        connect(_browser, SIGNAL(shibbolethCookieReceived(QNetworkCookie)),
                this, SLOT(slotShibbolethCookieReceived(QNetworkCookie)));
        connect(_browser, SIGNAL(viewHidden()),
                this, SLOT(slotViewHidden()));

        _browser->show();
        _browser->setFocus();
        wizard()->hide();
    } else {
        wizard()->show();
    }
}

void OwncloudShibbolethCredsPage::initializePage()
{
    WizardCommon::initErrorLabel(_ui.errorLabel);
    _afterInitialSetup = true;
    _ui.infoLabel->show();
    _ui.infoLabel->setText(tr("Please follow the steps on displayed page above"));
    _stage = INITIAL_STEP;
    _cookie = QNetworkCookie();
}

void OwncloudShibbolethCredsPage::disposeBrowser(bool later)
{
    if (_browser) {
        disconnect(_browser, SIGNAL(viewHidden()),
                   this, SLOT(slotViewHidden()));
        disconnect(_browser, SIGNAL(shibbolethCookieReceived(QNetworkCookie)),
                   this, SLOT(slotShibbolethCookieReceived(QNetworkCookie)));
        _browser->hide();
        if (later) {
            _browser->deleteLater();
        } else {
            delete _browser;
        }
        _browser = 0;
    }
}

void OwncloudShibbolethCredsPage::cleanupPage()
{
    disposeBrowser(false);
}

bool OwncloudShibbolethCredsPage::validatePage()
{
    switch (_stage) {
    case INITIAL_STEP:
        return false;

    case GOT_COOKIE:
        _stage = CHECKING;
        emit completeChanged();
        emit connectToOCUrl(field("OCUrl").toString().simplified());
        return false;

    case CHECKING:
        return false;

    case CONNECTED:
        return true;
    }

    return false;
}

int OwncloudShibbolethCredsPage::nextId() const
{
  return WizardCommon::Page_AdvancedSetup;
}

void OwncloudShibbolethCredsPage::setConnected( bool comp )
{
    if (comp) {
        _stage = CONNECTED;
    } else {
        // sets stage to INITIAL
        initializePage();
    }
    emit completeChanged();
    wizard()->show();
}

void OwncloudShibbolethCredsPage::setErrorString(const QString& err)
{
    if( err.isEmpty()) {
        _ui.errorLabel->setVisible(false);
    } else {
        initializePage();
        _ui.errorLabel->setVisible(true);
        _ui.errorLabel->setText(err);
    }
    emit completeChanged();
}

AbstractCredentials* OwncloudShibbolethCredsPage::getCredentials() const
{
    return new ShibbolethCredentials(_cookie);
}

void OwncloudShibbolethCredsPage::slotShibbolethCookieReceived(const QNetworkCookie& cookie)
{
    disposeBrowser(true);
    _stage = GOT_COOKIE;
    _cookie = cookie;
    _ui.infoLabel->setText("Please click \"Connect\" to check received Shibboleth session.");
    emit completeChanged();
    validatePage();
}

void OwncloudShibbolethCredsPage::slotViewHidden()
{
    disposeBrowser(true);
    wizard()->back();
    wizard()->show();
}

} // ns Mirall