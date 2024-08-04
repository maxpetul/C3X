
local ffi = require("ffi")

local USL_BLOCK_WIDTH = 8
local USL_BLOCK_HEIGHT = 4

ffi.cdef[[
typedef struct {
	unsigned short tiles[32]; // USL_BLOCK_WIDTH * USL_BLOCK_HEIGHT
} UShortBlock;
]]

local UShortLayer = {}
UShortLayer.__index = UShortLayer

function UShortLayer.new(mapWidth, mapHeight)
	local self = setmetatable({}, UShortLayer)
	self.mapWidth = mapWidth
	self.mapHeight = mapHeight
	self.widthInBlocks = math.ceil(mapWidth / (2 * USL_BLOCK_WIDTH))
	self.heightInBlocks = math.ceil(mapHeight / USL_BLOCK_HEIGHT)
	self.blocksSize = self.widthInBlocks * self.heightInBlocks * ffi.sizeof("UShortBlock")
	self.blocks = ffi.new("UShortBlock[?]", self.widthInBlocks * self.heightInBlocks)
	self.clear()
	return self
end

function UShortLayer:clear()
	ffi.fill(self.blocks, self.blocksSize, 0)
end

function UShortLayer:locate(x, y)
	local blockX = math.floor(x / (2 * USL_BLOCK_WIDTH))
	local rX = x % (2 * USL_BLOCK_WIDTH)
	local blockY = math.floor(y / USL_BLOCK_HEIGHT)
	local rY = y % USL_BLOCK_HEIGHT

	local blockIndex = blockY * self.widthInBlocks + blockX
	local tileIndex = rY * USL_BLOCK_WIDTH + math.floor(rX / 2)

	return blockIndex, tileIndex
end

function UShortLayer:get(x, y)
	local blockIndex, tileIndex = self:locate(x, y)
	return self.blocks[blockIndex].tiles[tileIndex]
end

function UShortLayer:set(x, y, val)
	local blockIndex, tileIndex = self:locate(x, y)
	self.blocks[blockIndex].tiles[tileIndex] = val
end
