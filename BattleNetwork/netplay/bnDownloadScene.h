#pragma once

#include <Swoosh/Activity.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>
#include <Swoosh/EmbedGLSL.h>
#include <Swoosh/Shaders.h>

#include <time.h>
#include <queue>
#include <typeinfo>
#include <SFML/Graphics.hpp>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>

#include "bnNetPlayPacketProcessor.h"
#include "../bnScene.h"
#include "../bnText.h"
#include "../../bnInputManager.h"
#include "../../bnDrawWindow.h"
#include "../../bnSceneNode.h"

struct DownloadSceneProps {
  std::vector<std::string> cardPackageHashes;
  std::vector<std::string> blockPackageHashes;
  std::string playerHash;
  Poco::Net::SocketAddress remoteAddress;
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;
  sf::Texture lastScreen;
  bool& downloadSuccess;
  unsigned& coinFlip;
  std::string& remotePlayerHash;
  std::vector<std::string>& remotePlayerBlocks;
};

class DownloadScene final : public Scene {
private:
  bool& downloadSuccess;
  bool downloadFlagSet{}, aborting{}, remoteSuccess{}, remoteHandshake{}, hasTradedData{};
  bool playerPackageRequested{}, cardPackageRequested{}, blockPackageRequested{};
  unsigned& coinFlip;
  unsigned mySeed{};
  frame_time_t abortingCountdown{frames(150)};
  size_t tries{}; //!< After so many attempts, quit the download...
  size_t packetAckId{};
  std::string playerHash;
  std::string& remotePlayerHash;
  std::vector<std::string>& remoteBlockHash;
  std::vector<std::string> playerCardPackageList, playerBlockPackageList;
  std::map<std::string, std::string> contentToDownload;
  Text label;
  sf::Sprite bg; // background
  sf::RenderTexture surface;
  sf::Texture lastScreen;
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;
  swoosh::glsl::FastGaussianBlur blur{ 10 };

  void RemoveFromDownloadList(const std::string& id);

  void SendHandshakeAck();
  bool AllTasksComplete();
  void SendPing(); //!< keep connections alive while clients download data

  // Notify remote of health
  void SendDownloadComplete(bool success);

  // Initiate trades
  void TradePlayerPackageData(const std::string& hash);
  void TradeCardPackageData(const std::vector<std::string>& hashes);
  void TradeBlockPackageData(const std::vector<std::string>& hashes);

  // Initiate requests
  void RequestPlayerPackageData(const std::string& hash);
  void RequestCardPackageList(const std::vector<std::string>& hashes);
  void RequestBlockPackageList(const std::vector<std::string>& hashes);

  // Handle recieve 
  void RecieveHandshake(const Poco::Buffer<char>& buffer);
  void RecieveTradePlayerPackageData(const Poco::Buffer<char>& buffer);
  void RecieveTradeCardPackageData(const Poco::Buffer<char>& buffer);
  void RecieveTradeBlockPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestPlayerPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestCardPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestBlockPackageData(const Poco::Buffer<char>& buffer);
  void RecieveDownloadComplete(const Poco::Buffer<char>& buffer);

  // Downloads
  void DownloadPlayerData(const Poco::Buffer<char>& buffer);

  template<typename PackageManagerType, typename ScriptedDataType>
  void DownloadPackageData(const Poco::Buffer<char>& buffer, PackageManagerType& pm);

  // Serializers
  std::vector<std::string> DeserializeListOfStrings(const Poco::Buffer<char>& buffer);
  Poco::Buffer<char> SerializeListOfStrings(NetPlaySignals header, const std::vector<std::string>& list);

  template<typename PackageManagerType>
  Poco::Buffer<char> SerializePackageData(const std::string& hash, NetPlaySignals header, PackageManagerType& pm);

  // Aux
  void Abort();
  void ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>& body);

public:
  void onUpdate(double elapsed) override final;
  void onDraw(sf::RenderTexture& surface) override final;
  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onStart() override;
  void onResume() override;
  void onEnd() override;

  /**
   * @brief Construct scene from previous screen's contents
   */
  DownloadScene(swoosh::ActivityController&, const DownloadSceneProps& props);
  ~DownloadScene();

};