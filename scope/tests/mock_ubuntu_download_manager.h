/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef _MOCK_UBUNTU_DOWNLOAD_MANAGER_H_
#define _MOCK_UBUNTU_DOWNLOAD_MANAGER_H_

namespace Ubuntu {
namespace DownloadManager {

class MockManager : public Manager {
 public:
  MOCK_METHOD1(createDownload,
      void(DownloadStruct downStruct));
  MOCK_METHOD3(createDownload,
      void(DownloadStruct downStruct, DownloadCb cb, DownloadCb errCb));
  MOCK_METHOD5(createDownload,
      void(StructList downs, const QString &algorithm, bool allowed3G, const QVariantMap &metadata, StringMap headers));
  MOCK_METHOD7(createDownload,
      void(StructList downs, const QString &algorithm, bool allowed3G, const QVariantMap &metadata, StringMap headers, GroupCb cb, GroupCb errCb));
};

}  // namespace DownloadManager
}  // namespace Ubuntu

#endif /* _MOCK_UBUNTU_DOWNLOAD_MANAGER_H_ */
