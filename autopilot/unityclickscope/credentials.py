# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2014 Canonical Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


import threading

from gi.repository import Accounts, GLib, Signon


class AccountManager(object):

    def __init__(self):
        self._manager = Accounts.Manager()

    def _start(self):
        self._main_loop = GLib.MainLoop()
        self._main_loop_thread = threading.Thread(
            target=self._main_loop.run)
        self._main_loop_thread.start()

    def _quit(self):
        self._main_loop.quit()
        self._main_loop_thread.join()

    def add_u1_credentials(self):
        self._start()
        account = self._manager.create_account('ubuntuone')
        account.set_enabled(True)
        account.store(self._on_account_stored, None)
        info = Signon.IdentityInfo.new()
        info.set_username('u1test+20140121@canonical.com')
        info.set_caption('u1test+20140121@canonical.com')
        info.set_secret('Hola123*', True)
        identity = Signon.Identity.new()
        identity.store_credentials_with_info(
            info, self._on_credentials_stored, account)
        import time
        time.sleep(5)
        account_service = Accounts.AccountService.new(account, None)
        auth_data = account_service.get_auth_data()
        identity = auth_data.get_credentials_id()
        method = auth_data.get_method()
        mechanism = auth_data.get_method()
        session_data = auth_data.get_parameters()
        session = Signon.AuthSession.new(identity, method)
        session.process(session_data, mechanism, self._on_login_process, None)
        self._quit()
        return account

    def _on_credentials_stored(self, identity, id, error, account):
        if error:
            self._quit()
            raise Exception(error.message)
        account.set_variant('CredentialsId', GLib.Variant('u', id))
        account.store(self._on_account_stored, None)

    def _on_account_stored(self, account, error, userdata):
        if error:
            self._quit()
            raise Exception(error.message)

    def _on_login_process(self, session, reply, error, userdata):
        if error:
            self._quit()
            raise Exception(error.message)

    def delete_account(self, account):
        self._start()
        account.delete()
        account.store(self._on_account_deleted, None)
        self._quit()

    def _on_account_deleted(self, account, error, userdata):
        if error:
            self._quit()
            raise Exception(error.message)
