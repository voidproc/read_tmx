# include <Siv3D.hpp> // Siv3D v0.6.15

namespace TMX
{
	struct Tile
	{
		int32 gid;
		int32 localId;
		int32 tilesetId;

		friend void Formatter(FormatData& formatData, const Tile& tile)
		{
			formatData.string += U"({}, {}, {})"_fmt(tile.gid, tile.localId, tile.tilesetId);
		}
	};

	struct TileLayer
	{
		int32 id;
		Size size;
		Grid<Tile> data;
	};

	struct Object
	{
		int32 id;
		Optional<int32> gid;
		Point pos;
		Size size;
	};

	struct ObjectLayer
	{
		int32 id;
		Array<Object> obj;
	};

	struct Tileset
	{
		int32 firstgid;
		FilePath source;
		FilePath imageSource;
		Size tileSize;
		int32 tileCount;
		int32 columns;
	};

	struct Map
	{
		Size size;
		Size tileSize;
		Array<TileLayer> tileLayers;
		Array<ObjectLayer> objLayers;
		Array<Tileset> tilesets;

		void dump()
		{
			Console << U"size:" << size;
			Console << U"tileSize:" << tileSize;
			Console << U"tileLayers.size:" << tileLayers.size();
			for (const auto& layer : tileLayers)
			{
				Console << U"layer.id:" << layer.id;
				Console << U"layer.size:" << layer.size;
				Console << U"layer.data:" << layer.data;
			}
			Console << U"objLayers.size:" << objLayers.size();
			for (const auto& layer : objLayers)
			{
				Console << U"layer.id:" << layer.id;
				for (const auto& obj : layer.obj)
				{
					Console << U"layer.obj.id:" << obj.id;
					Console << U"layer.obj.gid:" << obj.gid;
					Console << U"layer.obj.pos:" << obj.pos;
					Console << U"layer.obj.size:" << obj.size;
				}
			}
			Console << U"tilesets.size:" << tilesets.size();
			for (const auto& tileset : tilesets)
			{
				Console << U"tileset.firstgid:" << tileset.firstgid;
				Console << U"tileset.source:" << tileset.source;
				Console << U"tileset.imageSource:" << tileset.imageSource;
				Console << U"tileset.tileSize:" << tileset.tileSize;
				Console << U"tileset.tileCount:" << tileset.tileCount;
				Console << U"tileset.columns:" << tileset.columns;
			}
		}
	};

	namespace
	{
		int32 GetTilesetId(const Map& map, int32 gid)
		{
			for (const auto [index, tileset] : Indexed(map.tilesets))
			{
				if (InRange<int32>(gid, tileset.firstgid, tileset.firstgid + tileset.tileCount))
				{
					return static_cast<int32>(index);
				}
			}
			return 0;
		}
	}

	Map Load(const FilePath& tmxFilePath)
	{
		Map map;

		XMLReader xml{ tmxFilePath };

		map.size.x = ParseInt<int32>(xml.attribute(U"width").value_or(U"0"));
		map.size.y = ParseInt<int32>(xml.attribute(U"height").value_or(U"0"));
		map.tileSize.x = ParseInt<int32>(xml.attribute(U"tilewidth").value_or(U"0"));
		map.tileSize.y = ParseInt<int32>(xml.attribute(U"tileheight").value_or(U"0"));

		for (auto el = xml.firstChild(); el; el = el.nextSibling())
		{
			if (el.name() == U"layer")
			{
				auto& layer = map.tileLayers.emplace_back();
				layer.id = ParseInt<int32>(el.attribute(U"id").value_or(U"0"));
				layer.size.x = ParseInt<int32>(el.attribute(U"width").value_or(U"0"));
				layer.size.y = ParseInt<int32>(el.attribute(U"height").value_or(U"0"));

				for (auto el2 = el.firstChild(); el2; el2 = el2.nextSibling())
				{
					if (el2.name() == U"data" && el2.attribute(U"encoding").value_or(U"").lowercased() == U"csv")
					{
						String data = el2.text();
						layer.data = Grid<Tile>{
							layer.size,
							data.split(U',').map([](const String tileIdStr)
								{
									Tile tile;
									tile.gid = ParseInt<int32>(tileIdStr.trimmed());
									// ※localId, tilesetId は、すべてのプロパティを読み込んだあとでセットする
									return tile;
								}) };
					}
				}
			}
			else if (el.name() == U"objectgroup")
			{
				auto& layer = map.objLayers.emplace_back();
				layer.id = ParseInt<int32>(el.attribute(U"id").value_or(U"0"));

				for (auto el2 = el.firstChild(); el2; el2 = el2.nextSibling())
				{
					if (el2.name() == U"object")
					{
						auto& obj = layer.obj.emplace_back();
						obj.id = ParseInt<int32>(el2.attribute(U"id").value_or(U"0"));
						obj.gid = ParseOpt<int32>(el2.attribute(U"gid").value_or(U""));
						obj.pos.x = ParseInt<int32>(el2.attribute(U"x").value_or(U"0"));
						obj.pos.y = ParseInt<int32>(el2.attribute(U"y").value_or(U"0"));
						obj.size.x = ParseInt<int32>(el2.attribute(U"width").value_or(U"0"));
						obj.size.y = ParseInt<int32>(el2.attribute(U"height").value_or(U"0"));
					}
				}
			}
			else if (el.name() == U"tileset")
			{
				auto& tileset = map.tilesets.emplace_back();
				tileset.firstgid = ParseInt<int32>(el.attribute(U"firstgid").value_or(U"0"));
				tileset.source = el.attribute(U"source").value_or(U"");

				XMLReader tsx{ U"map/" + tileset.source};
				tileset.tileSize.x = ParseInt<int32>(tsx.attribute(U"tilewidth").value_or(U"0"));
				tileset.tileSize.y = ParseInt<int32>(tsx.attribute(U"tileheight").value_or(U"0"));
				tileset.tileCount = ParseInt<int32>(tsx.attribute(U"tilecount").value_or(U"0"));
				tileset.columns = ParseInt<int32>(tsx.attribute(U"columns").value_or(U"0"));

				for (auto tsxElem = tsx.firstChild(); tsxElem; tsxElem = tsxElem.nextSibling())
				{
					if (tsxElem.name() == U"image")
					{
						tileset.imageSource = tsxElem.attribute(U"source").value_or(U"");
					}
				}
			}
		}

		// tileLayers[*].data (Tile) の localId, tilesetId をセットする
		for (auto& layer : map.tileLayers)
		{
			for (auto& tile : layer.data)
			{
				tile.tilesetId = GetTilesetId(map, tile.gid);
				tile.localId = tile.gid - map.tilesets[tile.tilesetId].firstgid;
			}
		}

		return map;
	}
}

namespace
{
	void RegisterTilesetTextures(const TMX::Map& map)
	{
		for (const auto& tileset : map.tilesets)
		{
			TextureAsset::Register(tileset.imageSource, U"map/" + tileset.imageSource);
		}
	}

	TextureRegion TileTexture(const TMX::Map& map, const int32 tileX, const int32 tileY)
	{
		const auto& tile = map.tileLayers[0].data.at(tileY, tileX);
		const TMX::Tileset& tileset = map.tilesets[tile.tilesetId];
		const double x = (tile.localId % tileset.columns) * tileset.tileSize.x;
		const double y = (tile.localId / tileset.columns) * tileset.tileSize.y;
		return TextureAsset(tileset.imageSource)(x, y, map.tileSize);
	}
}

void Main()
{
	Scene::SetBackground(ColorF{ 0.6, 0.8, 0.7 });

	constexpr Size TileSize{ 16, 16 };
	constexpr Size SceneSize{ TileSize.x * 24, TileSize.y * 24 };
	Scene::Resize(SceneSize);
	Scene::SetResizeMode(ResizeMode::Keep);
	Window::Resize(SceneSize * 2);
	Scene::SetTextureFilter(TextureFilter::Nearest);

	TMX::Map map = TMX::Load(U"map/map.tmx");
	map.dump();

	RegisterTilesetTextures(map);

	while (System::Update())
	{
		for (int32 iy = 0; iy < Min(48, map.size.y); ++iy)
		{
			for (int32 ix = 0; ix < map.size.x; ++ix)
			{
				TileTexture(map, ix, iy).draw(ix * map.tileSize.x, iy * map.tileSize.y);
			}
		}
	}
}
