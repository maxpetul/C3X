
local ffi = require("ffi")

local USL_BLOCK_WIDTH, USL_BLOCK_HEIGHT = 8, 4
local TBL_BLOCK_WIDTH, TBL_BLOCK_HEIGHT = 16, 16

ffi.cdef[[
typedef struct {
	unsigned short tiles[32]; // USL_BLOCK_WIDTH * USL_BLOCK_HEIGHT
} UShortBlock;

typedef struct {
	unsigned int pieces[16];
} TwoBitBlock;
]]



-- UShortLayer stores one unsigned 16-bit integer per tile in a cache-efficient manner
local UShortLayer = {}
UShortLayer.__index = UShortLayer

function UShortLayer.new(mapWidth, mapHeight)
	local self = setmetatable({}, UShortLayer)
	self.widthInBlocks = math.ceil(mapWidth / (2 * USL_BLOCK_WIDTH))
	self.heightInBlocks = math.ceil(mapHeight / USL_BLOCK_HEIGHT)
	self.blocksCount = self.widthInBlocks * self.heightInBlocks
	self.blocks = ffi.new("UShortBlock[?]", self.blocksCount)
	self:clear()
	return self
end

function UShortLayer:clear()
	ffi.fill(self.blocks, self.blocksCount * ffi.sizeof("UShortBlock"), 0)
end

function UShortLayer:locate(x, y)
	local blockX, rX = math.floor(x / (2 * USL_BLOCK_WIDTH)), x % (2 * USL_BLOCK_WIDTH)
	local blockY, rY = math.floor(y /      USL_BLOCK_HEIGHT), y %      USL_BLOCK_HEIGHT

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


--[[

-- TwoBitLayer is like UShortLayer except it stores only two bits per tile
local TwoBitLayer = {}
TwoBitLayer.__index = TwoBitLayer

function TwoBitLayer.new(mapWidth, mapHeight)
	local self = setmetatable({}, TwoBitLayer)
	self.widthInBlocks = math.ceil(mapWidth / (2 * TBL_BLOCK_WIDTH))
	self.heightInBlocks = math.ceil(mapHeight / TBL_BLOCK_HEIGHT)
	self.blocksCount = self.widthInBlocks * self.heightInBlocks
	self.blocks = ffi.new("TwoBitBlock[?]", self.blocksCount)
	self:clear()
	return self
end

function TwoBitLayer:clear()
	ffi.fill(self.blocks, self.blocksCount * ffi.sizeof("TwoBitBlock"), 0)
end

function TwoBitLayer:locatePiece(x, y)
	-- Coordinates of the block (x, y) is in and the "remainder" coords inside the block
	local blockX, rX = math.floor(x / (2 * TBL_BLOCK_WIDTH)), x % (2 * TBL_BLOCK_WIDTH)
	local blockY, rY = math.floor(y /      TBL_BLOCK_HEIGHT), y %      TBL_BLOCK_HEIGHT

	local blockIndex = blockY * self.widthInBlocks + blockX
	local tileIndex = rY * TBL_BLOCK_WIDTH + math.floor(rX / 2)

	-- There are 16 tiles within each piece (32 bits / 2 bits/tile = 16 tiles)
	return blockIndex, math.floor(tileIndex / 16), tileIndex % 16
end

function TwoBitLayer:get(x, y)
	local blockIndex, pieceIndex, indexWithinPiece = self:locatePiece(x, y)
	local piece = self.blocks[blockIndex].pieces[pieceIndex]
	return (piece >> (indexWithinPiece * 2)) & 3
end

function TwoBitLayer:set(x, y, twoBitVal)
	local blockIndex, pieceIndex, indexWithinPiece = self:locatePiece(x, y)
	local piece = self.blocks[blockIndex].pieces[pieceIndex]
	local mask = 3 << (indexWithinPiece * 2)
	self.blocks[blockIndex].pieces[pieceIndex] = (piece & ~mask) | ((twoBitVal << (indexWithinPiece * 2)) & mask)
end

]]--
