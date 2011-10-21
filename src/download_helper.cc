/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
/* copyright --> */
#include "download_helper.h"

#include <iostream>
#include <algorithm>
#include <sstream>

#include "RequestGroup.h"
#include "Option.h"
#include "prefs.h"
#include "Metalink2RequestGroup.h"
#include "ProtocolDetector.h"
#include "ParameterizedStringParser.h"
#include "PStringBuildVisitor.h"
#include "UriListParser.h"
#include "DownloadContext.h"
#include "RecoverableException.h"
#include "DlAbortEx.h"
#include "message.h"
#include "fmt.h"
#include "FileEntry.h"
#include "LogFactory.h"
#include "File.h"
#include "util.h"
#include "array_fun.h"
#include "OptionHandler.h"
#include "ByteArrayDiskWriter.h"
#include "a2functional.h"
#include "ByteArrayDiskWriterFactory.h"
#include "MetadataInfo.h"
#ifdef ENABLE_BITTORRENT
# include "bittorrent_helper.h"
# include "BtConstants.h"
# include "UTMetadataPostDownloadHandler.h"
#endif // ENABLE_BITTORRENT

namespace aria2 {

const std::set<std::string>& listRequestOptions()
{
  static const std::string REQUEST_OPTIONS[] = {
    PREF_DIR->k,
    PREF_CHECK_INTEGRITY->k,
    PREF_CONTINUE->k,
    PREF_ALL_PROXY->k,
    PREF_ALL_PROXY_USER->k,
    PREF_ALL_PROXY_PASSWD->k,
    PREF_CONNECT_TIMEOUT->k,
    PREF_DRY_RUN->k,
    PREF_LOWEST_SPEED_LIMIT->k,
    PREF_MAX_FILE_NOT_FOUND->k,
    PREF_MAX_TRIES->k,
    PREF_NO_PROXY->k,
    PREF_OUT->k,
    PREF_PROXY_METHOD->k,
    PREF_REMOTE_TIME->k,
    PREF_SPLIT->k,
    PREF_TIMEOUT->k,
    PREF_HTTP_AUTH_CHALLENGE->k,
    PREF_HTTP_NO_CACHE->k,
    PREF_HTTP_USER->k,
    PREF_HTTP_PASSWD->k,
    PREF_HTTP_PROXY->k,
    PREF_HTTP_PROXY_USER->k,
    PREF_HTTP_PROXY_PASSWD->k,
    PREF_HTTPS_PROXY->k,
    PREF_HTTPS_PROXY_USER->k,
    PREF_HTTPS_PROXY_PASSWD->k,
    PREF_REFERER->k,
    PREF_ENABLE_HTTP_KEEP_ALIVE->k,
    PREF_ENABLE_HTTP_PIPELINING->k,
    PREF_HEADER->k,
    PREF_USE_HEAD->k,
    PREF_USER_AGENT->k,
    PREF_FTP_USER->k,
    PREF_FTP_PASSWD->k,
    PREF_FTP_PASV->k,
    PREF_FTP_PROXY->k,
    PREF_FTP_PROXY_USER->k,
    PREF_FTP_PROXY_PASSWD->k,
    PREF_FTP_TYPE->k,
    PREF_FTP_REUSE_CONNECTION->k,
    PREF_NO_NETRC->k,
    PREF_REUSE_URI->k,
    PREF_SELECT_FILE->k,
    PREF_BT_ENABLE_LPD->k,
    PREF_BT_EXTERNAL_IP->k,
    PREF_BT_HASH_CHECK_SEED->k,
    PREF_BT_MAX_OPEN_FILES->k,
    PREF_BT_MAX_PEERS->k,
    PREF_BT_METADATA_ONLY->k,
    PREF_BT_MIN_CRYPTO_LEVEL->k,
    PREF_BT_PRIORITIZE_PIECE->k,
    PREF_BT_REQUIRE_CRYPTO->k,
    PREF_BT_REQUEST_PEER_SPEED_LIMIT->k,
    PREF_BT_SAVE_METADATA->k,
    PREF_BT_SEED_UNVERIFIED->k,
    PREF_BT_STOP_TIMEOUT->k,
    PREF_BT_TRACKER_INTERVAL->k,
    PREF_BT_TRACKER_TIMEOUT->k,
    PREF_BT_TRACKER_CONNECT_TIMEOUT->k,
    PREF_ENABLE_PEER_EXCHANGE->k,
    PREF_FOLLOW_TORRENT->k,
    PREF_INDEX_OUT->k,
    PREF_MAX_UPLOAD_LIMIT->k,
    PREF_SEED_RATIO->k,
    PREF_SEED_TIME->k,
    PREF_FOLLOW_METALINK->k,
    PREF_METALINK_SERVERS->k,
    PREF_METALINK_LANGUAGE->k,
    PREF_METALINK_LOCATION->k,
    PREF_METALINK_OS->k,
    PREF_METALINK_VERSION->k,
    PREF_METALINK_PREFERRED_PROTOCOL->k,
    PREF_METALINK_ENABLE_UNIQUE_PROTOCOL->k,
    PREF_ALLOW_OVERWRITE->k,
    PREF_ALLOW_PIECE_LENGTH_CHANGE->k,
    PREF_ASYNC_DNS->k,
    PREF_AUTO_FILE_RENAMING->k,
    PREF_FILE_ALLOCATION->k,
    PREF_MAX_DOWNLOAD_LIMIT->k,
    PREF_NO_FILE_ALLOCATION_LIMIT->k,
    PREF_PARAMETERIZED_URI->k,
    PREF_REALTIME_CHUNK_CHECKSUM->k,
    PREF_REMOVE_CONTROL_FILE->k,
    PREF_ALWAYS_RESUME->k,
    PREF_MAX_RESUME_FAILURE_TRIES->k,
    PREF_HTTP_ACCEPT_GZIP->k,
    PREF_MAX_CONNECTION_PER_SERVER->k,
    PREF_MIN_SPLIT_SIZE->k,
    PREF_CONDITIONAL_GET->k,
    PREF_ENABLE_ASYNC_DNS6->k,
    PREF_BT_TRACKER->k,
    PREF_BT_EXCLUDE_TRACKER->k,
    PREF_RETRY_WAIT->k,
    PREF_METALINK_BASE_URI->k,
    PREF_PAUSE->k,
    PREF_STREAM_PIECE_SELECTOR->k,
    PREF_HASH_CHECK_ONLY->k,
    PREF_CHECKSUM->k,
    PREF_PIECE_LENGTH->k
  };
  static std::set<std::string> requestOptions
    (vbegin(REQUEST_OPTIONS), vend(REQUEST_OPTIONS));
  return requestOptions;
}

namespace {
void unfoldURI
(std::vector<std::string>& result, const std::vector<std::string>& args)
{
  ParameterizedStringParser p;
  PStringBuildVisitor v;
  for(std::vector<std::string>::const_iterator itr = args.begin(),
        eoi = args.end(); itr != eoi; ++itr) {
    v.reset();
    p.parse(*itr)->accept(v);
    result.insert(result.end(), v.getURIs().begin(), v.getURIs().end()); 
  }
}
} // namespace

namespace {
template<typename InputIterator>
void splitURI(std::vector<std::string>& result,
              InputIterator begin,
              InputIterator end,
              size_t numSplit,
              size_t maxIter)
{
  size_t numURIs = std::distance(begin, end);
  if(numURIs >= numSplit) {
    result.insert(result.end(), begin, end);
  } else if(numURIs > 0) {
    size_t num = std::min(numSplit/numURIs, maxIter);
    for(size_t i = 0; i < num; ++i) {
      result.insert(result.end(), begin, end);
    }
    if(num < maxIter) {
      result.insert(result.end(), begin, begin+(numSplit%numURIs));
    }
  }
}
} // namespace

namespace {
SharedHandle<RequestGroup> createRequestGroup
(const SharedHandle<Option>& option, const std::vector<std::string>& uris,
 bool useOutOption = false)
{
  SharedHandle<RequestGroup> rg(new RequestGroup(option));
  SharedHandle<DownloadContext> dctx
    (new DownloadContext
     (option->getAsInt(PREF_PIECE_LENGTH),
      0,
      useOutOption&&!option->blank(PREF_OUT)?
      util::applyDir(option->get(PREF_DIR), option->get(PREF_OUT)):A2STR::NIL));
  dctx->getFirstFileEntry()->setUris(uris);
  dctx->getFirstFileEntry()->setMaxConnectionPerServer
    (option->getAsInt(PREF_MAX_CONNECTION_PER_SERVER));
#ifdef ENABLE_MESSAGE_DIGEST
  const std::string& checksum = option->get(PREF_CHECKSUM);
  if(!checksum.empty()) {
    std::pair<std::string, std::string> p;
    util::divide(p, checksum, '=');
    util::lowercase(p.first);
    util::lowercase(p.second);
    dctx->setDigest(p.first, util::fromHex(p.second));
  }
#endif // ENABLE_MESSAGE_DIGEST
  rg->setDownloadContext(dctx);
  rg->setPauseRequested(option->getAsBool(PREF_PAUSE));
  removeOneshotOption(rg->getOption());
  return rg;
}
} // namespace

#if defined ENABLE_BITTORRENT || ENABLE_METALINK
namespace {
SharedHandle<MetadataInfo> createMetadataInfo(const std::string& uri)
{
  return SharedHandle<MetadataInfo>(new MetadataInfo(uri));
}
} // namespace

namespace {
SharedHandle<MetadataInfo> createMetadataInfoDataOnly()
{
  return SharedHandle<MetadataInfo>(new MetadataInfo());
}
} // namespace
#endif // ENABLE_BITTORRENT || ENABLE_METALINK

#ifdef ENABLE_BITTORRENT

namespace {
SharedHandle<RequestGroup>
createBtRequestGroup(const std::string& torrentFilePath,
                     const SharedHandle<Option>& option,
                     const std::vector<std::string>& auxUris,
                     const std::string& torrentData = "",
                     bool adjustAnnounceUri = true)
{
  SharedHandle<RequestGroup> rg(new RequestGroup(option));
  SharedHandle<DownloadContext> dctx(new DownloadContext());
  if(torrentData.empty()) {
    // may throw exception
    bittorrent::load(torrentFilePath, dctx, option, auxUris);
    rg->setMetadataInfo(createMetadataInfo(torrentFilePath));
  } else {
    // may throw exception
    bittorrent::loadFromMemory(torrentData, dctx, option, auxUris, "default");
    rg->setMetadataInfo(createMetadataInfoDataOnly());
  }
  if(adjustAnnounceUri) {
    bittorrent::adjustAnnounceUri(bittorrent::getTorrentAttrs(dctx), option);
  }
  dctx->setFileFilter(util::parseIntRange(option->get(PREF_SELECT_FILE)));
  std::istringstream indexOutIn(option->get(PREF_INDEX_OUT));
  std::map<size_t, std::string> indexPathMap =
    util::createIndexPathMap(indexOutIn);
  for(std::map<size_t, std::string>::const_iterator i = indexPathMap.begin(),
        eoi = indexPathMap.end(); i != eoi; ++i) {
    dctx->setFilePathWithIndex
      ((*i).first, util::applyDir(option->get(PREF_DIR), (*i).second));
  }
  rg->setDownloadContext(dctx);
  rg->setPauseRequested(option->getAsBool(PREF_PAUSE));
  // Remove "metalink" from Accept Type list to avoid server from
  // responding Metalink file for web-seeding URIs.
  util::removeMetalinkContentTypes(rg);
  removeOneshotOption(rg->getOption());
  return rg;
}
} // namespace

namespace {
SharedHandle<RequestGroup>
createBtMagnetRequestGroup(const std::string& magnetLink,
                           const SharedHandle<Option>& option)
{
  SharedHandle<RequestGroup> rg(new RequestGroup(option));
  SharedHandle<DownloadContext> dctx
    (new DownloadContext(METADATA_PIECE_SIZE, 0,
                         A2STR::NIL));
  // We only know info hash. Total Length is unknown at this moment.
  dctx->markTotalLengthIsUnknown();
  rg->setFileAllocationEnabled(false);
  rg->setPreLocalFileCheckEnabled(false);
  bittorrent::loadMagnet(magnetLink, dctx);
  SharedHandle<TorrentAttribute> torrentAttrs =
    bittorrent::getTorrentAttrs(dctx);
  bittorrent::adjustAnnounceUri(torrentAttrs, rg->getOption());
  dctx->getFirstFileEntry()->setPath(torrentAttrs->name);
  rg->setDownloadContext(dctx);
  rg->clearPostDownloadHandler();
  SharedHandle<UTMetadataPostDownloadHandler> utMetadataPostHandler
    (new UTMetadataPostDownloadHandler());
  rg->addPostDownloadHandler(utMetadataPostHandler);
  rg->setDiskWriterFactory
    (SharedHandle<DiskWriterFactory>(new ByteArrayDiskWriterFactory()));
  rg->setMetadataInfo(createMetadataInfo(magnetLink));
  rg->markInMemoryDownload();
  rg->setPauseRequested(option->getAsBool(PREF_PAUSE));
  removeOneshotOption(rg->getOption());
  return rg;
}
} // namespace

void createRequestGroupForBitTorrent
(std::vector<SharedHandle<RequestGroup> >& result,
 const SharedHandle<Option>& option,
 const std::vector<std::string>& uris,
 const std::string& torrentData,
 bool adjustAnnounceUri)
{
  std::vector<std::string> nargs;
  if(option->get(PREF_PARAMETERIZED_URI) == A2_V_TRUE) {
    unfoldURI(nargs, uris);
  } else {
    nargs = uris;
  }
  // we ignore -Z option here
  size_t numSplit = option->getAsInt(PREF_SPLIT);
  SharedHandle<RequestGroup> rg =
    createBtRequestGroup(option->get(PREF_TORRENT_FILE), option, nargs,
                         torrentData, adjustAnnounceUri);
  rg->setNumConcurrentCommand(numSplit);
  result.push_back(rg);
}

#endif // ENABLE_BITTORRENT

#ifdef ENABLE_METALINK
void createRequestGroupForMetalink
(std::vector<SharedHandle<RequestGroup> >& result,
 const SharedHandle<Option>& option,
 const std::string& metalinkData)
{
  if(metalinkData.empty()) {
    Metalink2RequestGroup().generate(result,
                                     option->get(PREF_METALINK_FILE),
                                     option,
                                     option->get(PREF_METALINK_BASE_URI));
  } else {
    SharedHandle<ByteArrayDiskWriter> dw(new ByteArrayDiskWriter());
    dw->setString(metalinkData);
    Metalink2RequestGroup().generate(result, dw, option,
                                     option->get(PREF_METALINK_BASE_URI));
  }
}
#endif // ENABLE_METALINK

namespace {
class AccRequestGroup {
private:
  std::vector<SharedHandle<RequestGroup> >& requestGroups_;
  ProtocolDetector detector_;
  SharedHandle<Option> option_;
  bool ignoreLocalPath_;
  bool throwOnError_;
public:
  AccRequestGroup(std::vector<SharedHandle<RequestGroup> >& requestGroups,
                  const SharedHandle<Option>& option,
                  bool ignoreLocalPath = false,
                  bool throwOnError = false):
    requestGroups_(requestGroups), option_(option),
    ignoreLocalPath_(ignoreLocalPath),
    throwOnError_(throwOnError)
  {}

  void
  operator()(const std::string& uri)
  {
    if(detector_.isStreamProtocol(uri)) {
      std::vector<std::string> streamURIs;
      size_t numIter = option_->getAsInt(PREF_MAX_CONNECTION_PER_SERVER);
      size_t numSplit = option_->getAsInt(PREF_SPLIT);
      size_t num = std::min(numIter, numSplit);
      for(size_t i = 0; i < num; ++i) {
        streamURIs.push_back(uri);
      }
      SharedHandle<RequestGroup> rg = createRequestGroup(option_, streamURIs);
      rg->setNumConcurrentCommand(numSplit);
      requestGroups_.push_back(rg);
    }
#ifdef ENABLE_BITTORRENT
    else if(detector_.guessTorrentMagnet(uri)) {
      SharedHandle<RequestGroup> group =
        createBtMagnetRequestGroup(uri, option_);
      requestGroups_.push_back(group);
    } else if(!ignoreLocalPath_ && detector_.guessTorrentFile(uri)) {
      try {
        requestGroups_.push_back
          (createBtRequestGroup(uri, option_, std::vector<std::string>()));
      } catch(RecoverableException& e) {
        if(throwOnError_) {
          throw;
        } else {
          // error occurred while parsing torrent file.
          // We simply ignore it.
          A2_LOG_ERROR_EX(EX_EXCEPTION_CAUGHT, e);
        }
      }
    } 
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
    else if(!ignoreLocalPath_ && detector_.guessMetalinkFile(uri)) {
      try {
        Metalink2RequestGroup().generate(requestGroups_, uri, option_,
                                         option_->get(PREF_METALINK_BASE_URI));
      } catch(RecoverableException& e) {
        if(throwOnError_) {
          throw;
        } else {
          // error occurred while parsing metalink file.
          // We simply ignore it.
          A2_LOG_ERROR_EX(EX_EXCEPTION_CAUGHT, e);
        }
      }
    }
#endif // ENABLE_METALINK
    else {
      if(throwOnError_) {
        throw DL_ABORT_EX(fmt(MSG_UNRECOGNIZED_URI, uri.c_str()));
      } else {
        A2_LOG_ERROR(fmt(MSG_UNRECOGNIZED_URI, uri.c_str()));
      }
    }
  }
};
} // namespace

namespace {
class StreamProtocolFilter {
private:
  ProtocolDetector detector_;
public:
  bool operator()(const std::string& uri) {
    return detector_.isStreamProtocol(uri);
  }
};
} // namespace

void createRequestGroupForUri
(std::vector<SharedHandle<RequestGroup> >& result,
 const SharedHandle<Option>& option,
 const std::vector<std::string>& uris,
 bool ignoreForceSequential,
 bool ignoreLocalPath,
 bool throwOnError)
{
  std::vector<std::string> nargs;
  if(option->get(PREF_PARAMETERIZED_URI) == A2_V_TRUE) {
    unfoldURI(nargs, uris);
  } else {
    nargs = uris;
  }
  if(!ignoreForceSequential && option->get(PREF_FORCE_SEQUENTIAL) == A2_V_TRUE) {
    std::for_each(nargs.begin(), nargs.end(),
                  AccRequestGroup(result, option, ignoreLocalPath,
                                  throwOnError));
  } else {
    std::vector<std::string>::iterator strmProtoEnd =
      std::stable_partition(nargs.begin(), nargs.end(), StreamProtocolFilter());
    // let's process http/ftp protocols first.
    if(nargs.begin() != strmProtoEnd) {
      size_t numIter = option->getAsInt(PREF_MAX_CONNECTION_PER_SERVER);
      size_t numSplit = option->getAsInt(PREF_SPLIT);
      std::vector<std::string> streamURIs;
      splitURI(streamURIs, nargs.begin(), strmProtoEnd, numSplit, numIter);
      SharedHandle<RequestGroup> rg =
        createRequestGroup(option, streamURIs, true);
      rg->setNumConcurrentCommand(numSplit);
      result.push_back(rg);
    }
    // process remaining URIs(local metalink, BitTorrent files)
    std::for_each(strmProtoEnd, nargs.end(),
                  AccRequestGroup(result, option, ignoreLocalPath,
                                  throwOnError));
  }
}

namespace {
void createRequestGroupForUriList
(std::vector<SharedHandle<RequestGroup> >& result,
 const SharedHandle<Option>& option,
 const std::string& filename)
{
  UriListParser p(filename);
  while(p.hasNext()) {
    std::vector<std::string> uris;
    SharedHandle<Option> tempOption(new Option());
    p.parseNext(uris, *tempOption.get());
    if(uris.empty()) {
      continue;
    }

    SharedHandle<Option> requestOption(new Option(*option.get()));
    requestOption->remove(PREF_OUT);
    for(std::set<std::string>::const_iterator i =
          listRequestOptions().begin(), eoi = listRequestOptions().end();
        i != eoi; ++i) {
      const Pref* pref = option::k2p(*i);
      if(tempOption->defined(pref)) {
        requestOption->put(pref, tempOption->get(pref));
      }
    }

    createRequestGroupForUri(result, requestOption, uris);
  }
}
} // namespace

void createRequestGroupForUriList
(std::vector<SharedHandle<RequestGroup> >& result,
 const SharedHandle<Option>& option)
{
  if(option->get(PREF_INPUT_FILE) == "-") {
    createRequestGroupForUriList(result, option, DEV_STDIN);
  } else {
    if(!File(option->get(PREF_INPUT_FILE)).isFile()) {
      throw DL_ABORT_EX
        (fmt(EX_FILE_OPEN, option->get(PREF_INPUT_FILE).c_str(),
             "No such file"));
    }
    createRequestGroupForUriList(result, option, option->get(PREF_INPUT_FILE));
  }
}

SharedHandle<MetadataInfo>
createMetadataInfoFromFirstFileEntry(const SharedHandle<DownloadContext>& dctx)
{
  if(dctx->getFileEntries().empty()) {
    return SharedHandle<MetadataInfo>();
  } else {
    std::vector<std::string> uris;
    dctx->getFileEntries()[0]->getUris(uris);
    if(uris.empty()) {
      return SharedHandle<MetadataInfo>();
    }
    return SharedHandle<MetadataInfo>(new MetadataInfo(uris[0]));
  }
}

void removeOneshotOption(const SharedHandle<Option>& option)
{
  option->remove(PREF_PAUSE);
}

} // namespace aria2
