#pragma once

#include "Status.h"
#include "ModuleString.h"
#include <vector>

namespace mttJson
{
	enum class MessageId
	{
		Init,
		Deinit,

		/*
			Request:
			{
				filepath : *string*	//ALT
				magnet : *string*	//ALT
			}

			Response:
			{
				hash : *string*
			}
		*/
		Add,

		/*
			Request:
			{
				hash : *string*
			}
		*/
		Start,

		/*
			Request:
			{
				hash : *string*
			}
		*/
		Stop,

		/*
			Request:
			{
				hash : *string*
				removeFiles : *bool*	//OPT
			}
		*/
		Remove,

		/*
			Response:
			{
				list : [
					{
						hash : *string*
						active : *bool*
					}, ...
				]
			}
		*/
		GetTorrents,

		/*
			Request:
			{
				hash : *string*
			}

			Response:
			{
				name : *string*
				size : *number*
				files : [
					{
						path : [ *string*, ...]
						size : *number*
					},...
				]
			}
		*/
		GetTorrentInfo,

		/*
			Request:
			{
				hash : *string*
			}

			Response:
			{
				name : *string*
				connectedPeers : *number*
				foundPeers : *number*
				downloaded : *number*
				uploaded : *number*
				downloadSpeed : *number*
				uploadSpeed : *number*
				progress : *float*
				selectionProgress : *float*
				status : *number*	//mtt::Status last error

				utmActive : *bool*			//OPT
				checkProgress : *float*		//OPT
			}
		*/
		GetTorrentStateInfo,
		GetPeersInfo,	//uint8_t[20], TorrentPeersInfo
		GetSourcesInfo,	//uint8_t[20], SourcesInfo
		GetMagnetLinkProgress,	//uint8_t[20],MagnetLinkProgress
		GetMagnetLinkProgressLogs,	//uint8_t[20],MagnetLinkProgressLogs
		GetSettings, //null, SettingsInfo
		SetSettings, //SettingsInfo, null
		RefreshSource, //SourceId, null
		GetTorrentFilesSelection, //SourceId, TorrentFilesSelection
		SetTorrentFilesSelection, //TorrentFilesSelectionRequest, null
		AddPeer,	//AddPeerRequest, null
		GetPiecesInfo, //uint8_t[20], PiecesInfo
	};
};
