#pragma once

#include "Status.h"
#include "ModuleString.h"
#include <vector>

namespace mttJson
{
	enum class MessageId
	{
		/*
			Request:	//OPT
			{
				programFolderPath : *string*	//folder with library data
				stateFolder : *string*			//folder with torrents state data
				maxPeersPerTrackerRequest : *number*

				dht : {
					defaultRootHosts : [ *string*, ...]		//host:port
					peersCheckInterval : *number*
					maxStoredAnnouncedPeers : *number*
					maxPeerValuesResponse : *number*
				}
			}
		*/
		Init,

		/*
		*/
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
				hash : [*string*, ...]
			}

			Response:
			{
				states : [
					{
						hash : *string*
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
				, ...]
			}
		*/
		GetTorrentsState,

		/*
			Request:
			{
				hash : *string*
			}

			Response:
			{
				peers: [
				{
					progress : *float*
					downloadSpeed : *number*
					uploadSpeed : *number*
					address : *string*
					client : *string*	//OPT
					country : *string*	//OPT
				},...
				]
			}
		*/
		GetPeersInfo,	//uint8_t[20], TorrentPeersInfo

		/*
			Request:
			{
				hash : *string*
			}

			Response:
			{
				sources: [
				{
					name : *string*
					peers : *number*
					seeds : *number*
					leechers : *number*
					nextCheck : *number*
					interval : *number*
					status : *string*	//Stopped, Ready, Offline, Connecting, Announcing, Announced
				},...
				]
			}
		*/
		GetSourcesInfo,	//uint8_t[20], SourcesInfo
		
		/*
			Request:
			{
				hash : *string*
			}

			Response:
			{
				count: *number*
				pieces: *string*	//0 = miss, 1 = have, per piece
				requests: [*number*]
			}
		*/
		GetPiecesInfo,

		/*
			Request:
			{
				hash : *string*
			}

			Response:
			{
				finished: *bool*
				receivedParts: *number*
				totalParts: *number*
			}
		*/
		GetMagnetLinkProgress,

		/*
			Request:
			{
				hash : *string*
				start : *number*	//first log index
			}

			Response:
			{
				logs : [
				*string*, ...
				]
			}
		*/
		GetMagnetLinkProgressLogs,

		/*
			Request:
			{
				hash : *string*
			}

			Response:
			{
				files : [
					{
						name : *string*
						selected : *bool*
						size : *number*
						pieceStart : *number*
						pieceEnd : *number*
					},...
				]
			}
		*/
		GetTorrentFilesSelection,


		/*
			Request:
			{
				hash : *string*
				selection : [ *bool*, ... ] 
			}
		*/
		SetTorrentFilesSelection,

		/*
			Response:
			{
				*string*	//json settings
			}
		*/
		GetSettings,

		/*
			Request:
			{
				*string*	//json settings
			}
		*/
		SetSettings,

		/*
			Request:
			{
				hash : *string*
				name : *string*
			}
		*/
		RefreshSource,

		/*
			Request:
			{
				hash : *string*
				address : *string*
			}
		*/
		AddPeer,
	};
};
