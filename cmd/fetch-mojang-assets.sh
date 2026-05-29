#!/bin/sh -eu
VERSIONS_URL=https://piston-meta.mojang.com/mc/game/version_manifest_v2.json
VERSION=b1.7.3

mkdir -p assets

echo FETCH version_manifest_v2.json
version_info="$(curl -s "$VERSIONS_URL" | jq '.versions[] | select(.id == "'$VERSION'")')"
version_url=$(echo "$version_info" | jq -r '.url')

echo FETCH $VERSION.json
manifest=assets/$VERSION.json
curl -s "$version_url" >$manifest

echo FETCH assets.json
asset_manifest="$(jq -r .assetIndex.url < $manifest)"
curl -s "$asset_manifest" >assets/assets.json

echo FETCH client.jar
client_jar="$(jq -r .downloads.client.url < $manifest)"
curl -s $client_jar >assets/client.jar
(
	cd assets
	unzip -o client.jar font.txt particles.png pack.png terrain.png \
		art/'*' \
		armor/'*' \
		environment/'*' \
		gui/'*' \
		item/'*' \
		misc/'*' \
		mob/'*' \
		terrain/'*' \
		font/'*' \
		title/'*' \
		lang/'*'
	rm -f client.jar
)

assets=$(jq -r '.objects | keys | .[]' <assets/assets.json)
IFS='
'
for asset in $assets
do
	hash="$(jq -r '.objects["'"$asset"'"].hash' <assets/assets.json)"
	echo $asset
	mkdir -p $(dirname assets/"$asset")
	curl -s \
		-o assets/"$asset" \
		"https://resources.download.minecraft.net/$(echo $hash | cut -c1-2)/${hash}"
done

