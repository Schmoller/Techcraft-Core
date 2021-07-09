TMP_DIR=$(mktemp -d)

curl -L https://www.dropbox.com/s/om7pkmunkyb54lj/TerrainTest.zip?dl=0 -o "$TMP_DIR/TerrainTest.zip"

mkdir -p assets/textures
unzip $TMP_DIR/TerrainTest.zip -d assets/textures