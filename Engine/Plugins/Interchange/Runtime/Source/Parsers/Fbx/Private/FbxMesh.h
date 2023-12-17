// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FbxAPI.h"
#include "FbxHelper.h"
#include "FbxInclude.h"

/** Forward declarations */
struct FMeshDescription;
class UInterchangeBaseNodeContainer;
class UInterchangeMeshNode;

namespace UE
{
	namespace Interchange
	{
		namespace Private
		{
			struct FMorphTargetAnimationBuildingData
			{
				double StartTime;
				double StopTime;
				UInterchangeMeshNode* InterchangeMeshNode;
				int32 GeometryIndex;
				int32 AnimationIndex;
				FbxAnimLayer* AnimLayer;
				int32 MorphTargetIndex;
				int32 ChannelIndex;
				FString MorphTargetNodeUid;

				FMorphTargetAnimationBuildingData(double InStartTime
						, double InStopTime
						, UInterchangeMeshNode* InInterchangeMeshNode
						, int32 InGeometryIndex
						, int32 InAnimationIndex
						, FbxAnimLayer* InAnimLayer
						, int32 InMorphTargetIndex
						, int32 InChannelIndex
						, FString InMorphTargetNodeUid)
					: StartTime(InStartTime)
					, StopTime(InStopTime)
					, InterchangeMeshNode(InInterchangeMeshNode)
					, GeometryIndex(InGeometryIndex)
					, AnimationIndex(InAnimationIndex)
					, AnimLayer(InAnimLayer)
					, MorphTargetIndex(InMorphTargetIndex)
					, ChannelIndex(InChannelIndex)
					, MorphTargetNodeUid(InMorphTargetNodeUid)
				{

				}
			};

			class FFbxMesh;

			class FMeshDescriptionImporter
			{
			public:
				FMeshDescriptionImporter(FFbxParser& InParser, FMeshDescription* InMeshDescription, FbxScene* InSDKScene, FbxGeometryConverter* InSDKGeometryConverter);
				
				/*
				 * Fill the mesh description using the Mesh parameter.
				 */
				bool FillStaticMeshDescriptionFromFbxMesh(FbxMesh* Mesh, const FTransform& MeshGlobalTransform);
				
				/*
				 * Fill the mesh description using the Mesh parameter and also fill the OutJointNodeUniqueIDs so the MeshDescription bone Index can be map to the correct interchange joint scene node.
				 */
				bool FillSkinnedMeshDescriptionFromFbxMesh(FbxMesh* Mesh, const FTransform& MeshGlobalTransform, TArray<FString>& OutJointUniqueNames);

				/*
				 * Fill the mesh description using the Shape parameter.
				 */
				bool FillMeshDescriptionFromFbxShape(FbxShape* Shape, const FTransform& MeshGlobalTransform);

				/**
				 * Add messages to the message log
				 */
				template <typename T>
				T* AddMessage(FbxGeometryBase* FbxNode) const
				{
					T* Item = Parser.AddMessage<T>();
					Item->MeshName = Parser.GetFbxHelper()->GetMeshName(FbxNode);
					Item->InterchangeKey = Parser.GetFbxHelper()->GetMeshUniqueID(FbxNode);
					return Item;
				}

			private:
				
				enum class EMeshType : uint8
				{
					None = 0, //No mesh type to import
					Static = 1, //static mesh
					Skinned = 2, //skinned mesh with joints
				};

				bool FillMeshDescriptionFromFbxMesh(FbxMesh* Mesh, const FTransform& MeshGlobalTransform, TArray<FString>& OutJointUniqueNames, EMeshType MeshType);
				bool IsOddNegativeScale(FbxAMatrix& TotalMatrix);
				
				//TODO move the real function from RenderCore to FVector, so we do not have to add render core to compute such a simple thing
				float FbxGetBasisDeterminantSign(const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis)
				{
					FMatrix Basis(
						FPlane(XAxis, 0),
						FPlane(YAxis, 0),
						FPlane(ZAxis, 0),
						FPlane(0, 0, 0, 1)
					);
					return (Basis.Determinant() < 0) ? -1.0f : +1.0f;
				}

				FFbxParser& Parser;
				FMeshDescription* MeshDescription;
				FbxScene* SDKScene;
				FbxGeometryConverter* SDKGeometryConverter;
				bool bInitialized = false;
			};

			class FMeshPayloadContext : public FPayloadContextBase
			{
			public:
				virtual ~FMeshPayloadContext() {}
				virtual FString GetPayloadType() const override { return TEXT("Mesh-PayloadContext"); }
				virtual bool FetchMeshPayloadToFile(FFbxParser& Parser, const FTransform& MeshGlobalTransform, const FString& PayloadFilepath) override;
				bool bIsSkinnedMesh = false;
				FbxMesh* Mesh = nullptr;
				FbxScene* SDKScene = nullptr;
				FbxGeometryConverter* SDKGeometryConverter = nullptr;
			};

			class FMorphTargetPayloadContext : public FPayloadContextBase
			{
			public:
				virtual ~FMorphTargetPayloadContext() {}
				virtual FString GetPayloadType() const override { return TEXT("MorphTarget-PayloadContext"); }
				virtual bool FetchMeshPayloadToFile(FFbxParser& Parser, const FTransform& MeshGlobalTransform, const FString& PayloadFilepath) override;
				FbxShape* Shape = nullptr;
				FbxScene* SDKScene = nullptr;
				FbxGeometryConverter* SDKGeometryConverter = nullptr;
			};

			class FFbxMesh
			{
			public:
				explicit FFbxMesh(FFbxParser& InParser)
					: Parser(InParser)
				{}

				void AddAllMeshes(FbxScene* SDKScene, FbxGeometryConverter* SDKGeometryConverter, UInterchangeBaseNodeContainer& NodeContainer, TMap<FString, TSharedPtr<FPayloadContextBase>>& PayloadContexts);
				static bool GetGlobalJointBindPoseTransform(FbxScene* SDKScene, FbxNode* Joint, FbxAMatrix& GlobalBindPoseJointMatrix);

				TArray<FMorphTargetAnimationBuildingData>& GetMorphTargetAnimationsBuildingData() { return MorphTargetAnimationsBuildingData; }
			protected:
				/** Add joint to the interchange mesh node joint dependencies. Return false if there is no valid joint (not a valid skinned mesh) */
				bool ExtractSkinnedMeshNodeJoints(FbxScene* SDKScene, UInterchangeBaseNodeContainer& NodeContainer, FbxMesh* Mesh, UInterchangeMeshNode* MeshNode);
				UInterchangeMeshNode* CreateMeshNode(UInterchangeBaseNodeContainer& NodeContainer, const FString& NodeName, const FString& NodeUniqueID);

			private:
				FFbxParser& Parser;

				//In order to appropriately identify the Skeleton Node Uids we have to process the MorphTarget animations once the hierarchy is processed
				TArray<FMorphTargetAnimationBuildingData> MorphTargetAnimationsBuildingData;
			};
		}//ns Private
	}//ns Interchange
}//ns UE