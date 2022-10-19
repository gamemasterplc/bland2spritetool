#define _CRT_SECURE_NO_WARNINGS //Shut up Visual Studio
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "tinyxml2.h"

struct AnimFrame {
    std::string sprite_name;
    uint8_t delay;
    uint8_t max_delay; //Must be greater than delay to apply delay randomization
    float x_scale;
    float y_scale;
    float x;
    float y;
    int16_t angle; //0-4095 equivalent to a circle
};

struct Image {
    uint16_t texture_id;
    uint16_t num_palettes; //Treated as consecutive texture_ids starting from texture_id
    int16_t x;
    int16_t y;
    uint16_t src_x;
    uint16_t src_y;
    uint16_t w;
    uint16_t h;
    uint8_t alpha_mode; //0=100% alpha, 1=75% alpha, 2=50% alpha, 3=25% alpha
    int16_t angle; //0-4095 equivalent to a circle
    uint8_t blend_mode; //0=normal, 1=additive, 2=mask, 3=none
    bool bilinear;
    uint8_t flip; //0x1=x-flip, 0x2=y-flip
};

struct Sprite {
    std::string name;
    int16_t min_x;
    int16_t min_y;
    int16_t max_x;
    int16_t max_y;
    std::vector<Image> images;
};

struct SpriteHeader {
    uint16_t sprite_count;
    uint16_t anim_count;
    uint16_t frame_count;
    uint16_t image_count;
    uint32_t sprite_ofs;
    uint32_t anim_ofs;
    uint32_t frame_ofs;
    uint32_t image_ofs;
};

std::vector<std::vector<AnimFrame>> anim_list;
std::vector<Sprite> sprite_list;

void XMLCheck(tinyxml2::XMLError error)
{
    if (error != tinyxml2::XML_SUCCESS) {
        //Terminate if not successful
        std::cout << "tinyxml2 error " << tinyxml2::XMLDocument::ErrorIDToName(error) << std::endl;
        exit(1);
    }
}

tinyxml2::XMLError QueryAttributeU8(tinyxml2::XMLElement *element, const char *name, uint8_t *value)
{
    uint32_t temp;
    tinyxml2::XMLError error = element->QueryAttribute(name, &temp);
    if (error == tinyxml2::XML_SUCCESS) {
        //Write value if successful
        *value = temp;
    }
    return error;
}

tinyxml2::XMLError QueryAttributeU16(tinyxml2::XMLElement *element, const char *name, uint16_t *value)
{
    uint32_t temp;
    tinyxml2::XMLError error = element->QueryAttribute(name, &temp);
    if (error == tinyxml2::XML_SUCCESS) {
        //Write value if successful
        *value = temp;
    }
    return error;
}

tinyxml2::XMLError QueryAttributeS16(tinyxml2::XMLElement *element, const char *name, int16_t *value)
{
    int32_t temp;
    tinyxml2::XMLError error = element->QueryAttribute(name, &temp);
    if (error == tinyxml2::XML_SUCCESS) {
        //Write value if successful
        *value = temp;
    }
    return error;
}

void SetSeek(FILE *file, size_t ofs)
{
    fseek(file, ofs, SEEK_SET);
}

size_t GetSeek(FILE *file)
{
    return ftell(file);
}

void FileSkip(FILE *file, size_t bytes)
{
    fseek(file, bytes, SEEK_CUR);
}

size_t GetFileSize(FILE *file)
{
    //Seek to end and restore file pointer
    size_t old_seek = GetSeek(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    SetSeek(file, old_seek);
    return size;
}

uint8_t ReadU8(FILE *file)
{
    uint8_t temp;
    fread(&temp, 1, 1, file);
    return temp;
}

int8_t ReadS8(FILE *file)
{
    //Same as reading unsigned version
    return ReadU8(file);
}

bool ReadBool(FILE *file)
{
    //Bools are implemented as unsigned 8-bit integers in c++
    return ReadU8(file);
}

uint16_t ReadU16(FILE *file)
{
    uint8_t temp[2];
    fread(&temp, 1, 2, file);
    //Return bytes in little-endian order
    return (temp[1] << 8) | temp[0];
}

int16_t ReadS16(FILE *file)
{
    return ReadU16(file);
}

uint32_t ReadU32(FILE *file)
{
    uint8_t temp[4];
    fread(&temp, 1, 4, file);
    //Return bytes in little-endian order
    return (temp[3] << 24) | (temp[2] << 16) | (temp[1] << 8) | temp[0];
}

int32_t ReadS32(FILE *file)
{
    return ReadU32(file);
}

float ReadFloat(FILE *file)
{
    int32_t raw = ReadS32(file);
    return *(float *)(&raw); //Reinterpret 4 bytes as float
}

void WriteU8(FILE *file, uint8_t value)
{
    fwrite(&value, 1, 1, file);
}

void WriteS8(FILE *file, int8_t value)
{
    fwrite(&value, 1, 1, file);
}

void WriteBool(FILE *file, bool value)
{
    //Bools are implemented as 8-bit integers in c++
    WriteU8(file, value);
}

void WriteU16(FILE *file, uint16_t value)
{
    uint8_t temp[2];
    //Write bytes in little-endian order
    temp[1] = value >> 8;
    temp[0] = value & 0xFF;
    fwrite(temp, 2, 1, file);
}

void WriteS16(FILE *file, int16_t value)
{
    int8_t temp[2];
    //Write bytes in little-endian order
    temp[1] = value >> 8;
    temp[0] = value & 0xFF;
    fwrite(temp, 2, 1, file);
}

void WriteU32(FILE *file, uint32_t value)
{
    uint8_t temp[4];
    //Write bytes in little-endian order
    temp[3] = value >> 24;
    temp[2] = (value >> 16) & 0xFF;
    temp[1] = (value >> 8) & 0xFF;
    temp[0] = value & 0xFF;
    fwrite(temp, 4, 1, file);
}

void WriteS32(FILE *file, int32_t value)
{
    int8_t temp[4];
    //Write bytes in little-endian order
    temp[3] = value >> 24;
    temp[2] = (value >> 16) & 0xFF;
    temp[1] = (value >> 8) & 0xFF;
    temp[0] = value & 0xFF;
    fwrite(temp, 4, 1, file);
}

void WriteFloat(FILE *file, float value)
{
    WriteS32(file, *(int32_t *)&value); //Write the bits of the float to the file
}

void ReadSpriteHeader(FILE *file, SpriteHeader &header)
{
    //Read count fields
    header.sprite_count = ReadU16(file);
    header.anim_count = ReadU16(file);
    header.frame_count = ReadU16(file);
    header.image_count = ReadU16(file);
    //Read offset fields
    header.sprite_ofs = ReadU32(file);
    header.anim_ofs = ReadU32(file);
    header.frame_ofs = ReadU32(file);
    header.image_ofs = ReadU32(file);
}

bool VerifySpriteHeader(SpriteHeader &header, size_t file_size)
{
    //Check if end of each section exceeds end of file
    bool anim_valid = ((header.anim_ofs) + (4 * header.anim_count)) <= file_size;
    bool frame_valid = ((header.frame_ofs) + (28 * header.frame_count)) <= file_size;
    bool sprite_valid = ((header.sprite_ofs) + (12 * header.sprite_count)) <= file_size;
    bool image_valid = ((header.image_ofs) + (28 * header.image_count)) <= file_size;
    return anim_valid && frame_valid && sprite_valid && image_valid;
}

void ReadAnims(FILE *file, SpriteHeader &header)
{
    //Seek to animations
    SetSeek(file, header.anim_ofs);
    for (uint16_t i = 0; i < header.anim_count; i++) {
        //Read animation frame range
        uint16_t start_frame = ReadU16(file);
        uint16_t num_frames = ReadU16(file);
        std::vector<AnimFrame> anim;
        size_t old_seek = GetSeek(file);
        //Read frames for each animation
        SetSeek(file, header.frame_ofs + (start_frame * 28));
        for (uint16_t j = 0; j < num_frames; j++) {
            AnimFrame frame;
            //Read sprite index
            uint16_t sprite_idx = ReadU16(file);
            //Convert sprite index to name
            frame.sprite_name = "sprite" + std::to_string(sprite_idx);
            //Read delay fields
            frame.delay = ReadU8(file);
            frame.max_delay = ReadU8(file);
            //Read scale fields
            frame.x_scale = ReadFloat(file);
            frame.y_scale = ReadFloat(file);
            //Read position fields
            frame.x = ReadFloat(file);
            frame.y = ReadFloat(file);
            //Read angle
            frame.angle = ReadS16(file);
            FileSkip(file, 6); //Skip animation ID, next frame, and padding field
            anim.push_back(frame); //Add frame to animation
        }
        //Enable reading next animation
        SetSeek(file, old_seek);
        anim_list.push_back(anim);
    }
}

void ReadSprites(FILE *file, SpriteHeader &header)
{
    SetSeek(file, header.sprite_ofs);
    for (uint16_t i = 0; i < header.sprite_count; i++) {
        Sprite sprite;
        //Read sprite image ranges
        uint16_t start_image = ReadU16(file);
        uint16_t num_images = ReadU16(file);
        //Convert sprite index to name
        sprite.name = "sprite" + std::to_string(i);
        //Read sprite bounding rectangle
        sprite.min_x = ReadS16(file);
        sprite.min_y = ReadS16(file);
        sprite.max_x = ReadS16(file);
        sprite.max_y = ReadS16(file);
        size_t old_seek = GetSeek(file);
        //Read images for sprite
        SetSeek(file, header.image_ofs + (start_image * 28));
        for (uint16_t j = 0; j < num_images; j++) {
            Image image;
            image.texture_id = ReadU16(file); //Read texture ID
            image.num_palettes = ReadU16(file); //Read palette count
            //Read image position
            image.x = ReadS16(file);
            image.y = ReadS16(file);
            //Read image source position
            image.src_x = ReadU16(file);
            image.src_y = ReadU16(file);
            //Read image size
            image.w = ReadU16(file);
            image.h = ReadU16(file);
            FileSkip(file, 1); //Skip unknown field
            image.alpha_mode = ReadU8(file); //Read image alpha mode
            FileSkip(file, 2); //Skip 2 unknown fields
            image.angle = ReadS16(file); //Read image angle
            image.blend_mode = ReadU8(file); //Read image blend mode
            image.bilinear = ReadBool(file); //Read image bilinear filter flag
            image.flip = ReadU8(file); //Read image flip flags
            FileSkip(file, 3); //Skip 3 unknown fields
            sprite.images.push_back(image);
        }
        SetSeek(file, old_seek); //Enable reading next sprite
        sprite_list.push_back(sprite);
    }
}

float GetImageAlpha(uint8_t value)
{
    if (value >= 4) {
        //Game treats image modes greater than 4 as full alpha
        return 1.0f;
    }
    //Game treats values 0-3 as 100% to 25%
    return (4 - value) * 0.25f;
}

uint8_t GetAlphaModeValue(float alpha)
{
    //Use midpoint check for alpha value
    if (alpha > 0.875f) {
        return 0;
    } else if (alpha > 0.625f) {
        return 1;
    } else if (alpha > 0.375f) {
        return 2;
    } else {
        return 3;
    }
}

uint8_t GetBlendModeValue(const char *name)
{
    std::string name_temp = name;
    std::string mode_names[3] = { "normal", "additive", "mask" };
    for (size_t i = 0; i < 3; i++) {
        if (name_temp == mode_names[i]) {
            //Found blend mode with proper name
            return i;
        }
    }
    //Use none blend mode if not found
    return 3;
}

const char *GetBlendModeName(uint8_t value)
{
    const char *names[3] = { "normal", "additive", "mask" };
    if (value >= 3) {
        //Replace invalid blend modes with none
        return "none";
    }
    //Read blend mode name from table
    return names[value];
}

void WriteSpriteXML(std::string out_file)
{
    tinyxml2::XMLDocument document;
    //Add root element
    tinyxml2::XMLElement *root = document.NewElement("spritedata");
    document.InsertFirstChild(root);
    //Write animation sequences
    for (size_t i = 0; i < anim_list.size(); i++) {
        tinyxml2::XMLElement *anim_root = document.NewElement("anim");
        //Write animation frames
        for (size_t j = 0; j < anim_list[i].size(); j++) {
            tinyxml2::XMLElement *frame = document.NewElement("frame");
            //Write sprite name
            frame->SetAttribute("sprite", anim_list[i][j].sprite_name.c_str());
            //Write non-default delay
            if (anim_list[i][j].delay != 1) {
                frame->SetAttribute("delay", anim_list[i][j].delay);
            }
            //Write non-default max delay
            if (anim_list[i][j].max_delay != 0) {
                frame->SetAttribute("delay_range", anim_list[i][j].max_delay);
            }
            //Write non-default scale
            if (anim_list[i][j].x_scale != 1.0f) {
                frame->SetAttribute("x_scale", anim_list[i][j].x_scale);
            }
            if (anim_list[i][j].y_scale != 1.0f) {
                frame->SetAttribute("y_scale", anim_list[i][j].y_scale);
            }
            //Write non-default position
            if (anim_list[i][j].x != 0.0f) {
                frame->SetAttribute("x", anim_list[i][j].x);
            }
            if (anim_list[i][j].y != 0.0f) {
                frame->SetAttribute("y", anim_list[i][j].y);
            }
            //Write non-default angle
            if (anim_list[i][j].angle != 0) {
                frame->SetAttribute("angle", anim_list[i][j].angle);
            }
            //Add animation frame to animation
            anim_root->InsertEndChild(frame);
        }
        //Add animation to XML
        root->InsertEndChild(anim_root);
    }
    //Write sprites
    for (size_t i = 0; i < sprite_list.size(); i++) {
        tinyxml2::XMLElement *sprite = document.NewElement("sprite");
        //Write sprite name
        sprite->SetAttribute("name", sprite_list[i].name.c_str());
        //Write sprite images
        for (size_t j = 0; j < sprite_list[i].images.size(); j++) {
            tinyxml2::XMLElement *image = document.NewElement("image");
            image->SetAttribute("texture_id", sprite_list[i].images[j].texture_id);
            //Write number of palettes if more than 1
            if (sprite_list[i].images[j].num_palettes > 1) {
                image->SetAttribute("num_palettes", sprite_list[i].images[j].num_palettes);
            }
            //Write source position of image
            image->SetAttribute("src_x", sprite_list[i].images[j].src_x);
            image->SetAttribute("src_y", sprite_list[i].images[j].src_y);
            //Write position of image
            image->SetAttribute("x", sprite_list[i].images[j].x);
            image->SetAttribute("y", sprite_list[i].images[j].y);
            //Write size of image
            image->SetAttribute("w", sprite_list[i].images[j].w);
            image->SetAttribute("h", sprite_list[i].images[j].h);
            //Write non-default alpha mode
            if (sprite_list[i].images[j].alpha_mode != 0) {
                image->SetAttribute("alpha", GetImageAlpha(sprite_list[i].images[j].alpha_mode));
            }
            //Write non-zero angle
            if (sprite_list[i].images[j].angle != 0) {
                image->SetAttribute("angle", sprite_list[i].images[j].angle);
            }
            //Write non-default blend mode
            if (sprite_list[i].images[j].blend_mode != 0) {
                image->SetAttribute("blend_mode", GetBlendModeName(sprite_list[i].images[j].blend_mode));
            }
            //Write bilinear flag if used
            if (sprite_list[i].images[j].bilinear) {
                image->SetAttribute("bilinear", sprite_list[i].images[j].bilinear);
            }
            //Write flip flags
            if (sprite_list[i].images[j].flip & 0x1) {
                image->SetAttribute("flip_x", (sprite_list[i].images[j].flip & 0x1) != 0);
            }
            if (sprite_list[i].images[j].flip & 0x2) {
                image->SetAttribute("flip_y", (sprite_list[i].images[j].flip & 0x2) != 0);
            }
            //Add image to sprite
            sprite->InsertEndChild(image);
        }
        //Add sprite to XML
        root->InsertEndChild(sprite);
    }
    //Write out XML
    XMLCheck(document.SaveFile(out_file.c_str()));
}

void DumpSprite(std::string in_file, std::string out_file)
{
    //Try to open file
    FILE *file = fopen(in_file.c_str(), "rb");
    if (!file) {
        //Die if failed to open
        std::cout << "Failed to open " << in_file << " for reading." << std::endl;
        exit(1);
    }
    //Read and verify sprite header
    SpriteHeader header;
    ReadSpriteHeader(file, header);
    if (!VerifySpriteHeader(header, GetFileSize(file))) {
        //Die if verification fails
        std::cout << "Invalid sprite file." << std::endl;
        exit(1);
    }
    //Read animation and sprite data
    ReadAnims(file, header);
    ReadSprites(file, header);
    fclose(file);
    //Write output
    WriteSpriteXML(out_file);
}

void ParseSprites(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *root)
{
    //Iterate through sprite elements in XML
    tinyxml2::XMLElement *sprite_element = root->FirstChildElement("sprite");
    while (sprite_element) {
        Sprite sprite;
        //Query sprite name
        const char *sprite_name;
        XMLCheck(sprite_element->QueryAttribute("name", &sprite_name));
        sprite.name = sprite_name;
        sprite.min_x = sprite.min_y = sprite.max_x = sprite.max_y = 0; //Zero out sprite rectangle
        //Iterate through image elements in XML
        tinyxml2::XMLElement *image_element = sprite_element->FirstChildElement("image"); 
        while (image_element) {
            Image image;
            float alpha_value = 1.0f; //Image is opaque
            const char *blend_mode_name = "normal"; //Use normal blend mode by default
            bool flip_x = false; //No X-Flip by default
            bool flip_y = false; //No Y-Flip by default
            //Query texture ID
            XMLCheck(QueryAttributeU16(image_element, "texture_id", &image.texture_id));
            //Query palette count
            image.num_palettes = 1; //Always have base palette
            QueryAttributeU16(image_element, "num_palettes", &image.num_palettes);
            //Query position of image
            XMLCheck(QueryAttributeS16(image_element, "x", &image.x));
            XMLCheck(QueryAttributeS16(image_element, "y", &image.y));
            //Query source position from texture
            XMLCheck(QueryAttributeU16(image_element, "src_x", &image.src_x));
            XMLCheck(QueryAttributeU16(image_element, "src_y", &image.src_y));
            //Query size of image
            XMLCheck(QueryAttributeU16(image_element, "w", &image.w));
            XMLCheck(QueryAttributeU16(image_element, "h", &image.h));
            //Query alpha mode
            image_element->QueryAttribute("alpha", &alpha_value);
            image.alpha_mode = GetAlphaModeValue(alpha_value);
            //Query angle
            image.angle = 0;
            QueryAttributeS16(image_element, "angle", &image.angle);
            //Query blend mode
            image_element->QueryAttribute("blend_mode", &blend_mode_name);
            image.blend_mode = GetBlendModeValue(blend_mode_name);
            //Query bilinear field
            image.bilinear = false;
            image_element->QueryAttribute("bilinear", &image.bilinear);
            //Query flip fields
            image_element->QueryAttribute("flip_x", &flip_x);
            image_element->QueryAttribute("flip_y", &flip_y);
            image.flip = 0;
            //Set X Flip bit if enabled
            if (flip_x) {
                image.flip |= 0x1;
            }
            //Set Y Flip bit if enabled
            if (flip_y) {
                image.flip |= 0x2;
            }
            //Add image to sprite image list
            sprite.images.push_back(image);
            image_element = image_element->NextSiblingElement("image"); //Next image
        }
        //Add sprite to global sprite list
        sprite_list.push_back(sprite);
        sprite_element = sprite_element->NextSiblingElement("sprite"); //Next sprite
    }
}

void ParseAnims(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *root)
{
    //Read animations
    tinyxml2::XMLElement *anim_element = root->FirstChildElement("anim");
    while (anim_element) {
        std::vector<AnimFrame> anim;
        //Read frames of animation
        tinyxml2::XMLElement *frame_element = anim_element->FirstChildElement("frame");
        while (frame_element) {
            AnimFrame frame;
            const char *sprite_name_value;
            //Read frame sprite name
            XMLCheck(frame_element->QueryAttribute("sprite", &sprite_name_value));
            frame.sprite_name = sprite_name_value;
            //Read frame delay
            frame.delay = 1;
            QueryAttributeU8(frame_element, "delay", &frame.delay);
            //Read max delay
            frame.max_delay = 0;
            QueryAttributeU8(frame_element, "max_delay", &frame.max_delay);
            //Read frame scale
            frame.x_scale = frame.y_scale = 1.0f;
            frame_element->QueryAttribute("x_scale", &frame.x_scale);
            frame_element->QueryAttribute("y_scale", &frame.y_scale);
            //Read frame position
            frame.x = frame.y = 0.0f;
            frame_element->QueryAttribute("x", &frame.x);
            frame_element->QueryAttribute("y", &frame.y);
            //Read frame angle
            frame.angle = 0;
            QueryAttributeS16(frame_element, "angle", &frame.angle);
            //Add frame to animation
            anim.push_back(frame);
            //Go to next frame
            frame_element = frame_element->NextSiblingElement("frame");
        }
        //Add animation to global list
        anim_list.push_back(anim);
        //Go to next animation
        anim_element = anim_element->NextSiblingElement("anim");
    }
}

bool FindSprite(std::string name, uint16_t *idx)
{
    for (size_t i = 0; i < sprite_list.size(); i++) {
        if (sprite_list[i].name == name) {
            //Write index if found
            *idx = i;
            return true;
        }
    }
    //Return false if not found
    return false;
}

void CalcSpriteBoundingRects()
{
    for (size_t i = 0; i < sprite_list.size(); i++) {
        int16_t min_x, max_x, min_y, max_y;
        //Set defaults for bounding rect in sprite
        min_x = min_y = INT16_MAX;
        max_x = max_y = INT16_MIN;
        for (size_t j = 0; j < sprite_list[i].images.size(); j++) {
            //Get image rectangle
            int16_t x = sprite_list[i].images[j].x;
            int16_t y = sprite_list[i].images[j].y;
            int16_t w = sprite_list[i].images[j].w;
            int16_t h = sprite_list[i].images[j].h;
            //Update bounding rect based on image rectangle
            if (x < min_x) {
                min_x = x;
            }
            if (y < min_y) {
                min_y = y;
            }
            if ((x + w) > max_x) {
                max_x = x + w;
            }
            if ((y + h) > max_y) {
                max_y = y + h;
            }
        }
        //Set sprite bounding rectangle
        sprite_list[i].min_x = min_x;
        sprite_list[i].min_y = min_y;
        sprite_list[i].max_x = max_x;
        sprite_list[i].max_y = max_y;
    }
}

void CreateSpriteHeader(SpriteHeader &header)
{
    //Set sprite header info
    header.sprite_ofs = 0x18;
    header.sprite_count = sprite_list.size();
    //Set animation header info
    header.anim_ofs = header.sprite_ofs + (header.sprite_count * 12);
    header.anim_count = anim_list.size();
    //Set frame header info
    header.frame_ofs = header.anim_ofs + (header.anim_count * 4);
    //Calculate frame count
    header.frame_count = 0;
    for (size_t i = 0; i < anim_list.size(); i++) {
        header.frame_count += anim_list[i].size();
    }
    //Set image header info
    header.image_ofs = header.frame_ofs + (header.frame_count * 28);
    //Calculate image count
    header.image_count = 0;
    for (size_t i = 0; i < sprite_list.size(); i++) {
        header.image_count += sprite_list[i].images.size();
    }
}

void WriteSpriteHeader(FILE *file, SpriteHeader &header)
{
    //Write count fields
    WriteU16(file, header.sprite_count);
    WriteU16(file, header.anim_count);
    WriteU16(file, header.frame_count);
    WriteU16(file, header.image_count);
    //Write offset fields
    WriteU32(file, header.sprite_ofs);
    WriteU32(file, header.anim_ofs);
    WriteU32(file, header.frame_ofs);
    WriteU32(file, header.image_ofs);
}

void WriteSprites(FILE *file)
{
    uint16_t start_image = 0; //Start with image 0
    //Loop over sprites
    for (size_t i = 0; i < sprite_list.size(); i++) {
        //Write image range
        WriteU16(file, start_image);
        WriteU16(file, sprite_list[i].images.size());
        //Write bounding rectangle
        WriteS16(file, sprite_list[i].min_x);
        WriteS16(file, sprite_list[i].min_y);
        WriteS16(file, sprite_list[i].max_x);
        WriteS16(file, sprite_list[i].max_y);
        //Get next starting image
        start_image += sprite_list[i].images.size();
    }
}

void WriteAnims(FILE *file)
{
    uint16_t start_frame = 0; //Start with frame 0
    //Loop over animations
    for (size_t i = 0; i < anim_list.size(); i++) {
        //Write animation frame range
        WriteU16(file, start_frame);
        WriteU16(file, anim_list[i].size());
        //Get next starting frame
        start_frame += anim_list[i].size();
    }
}

void WriteAnimFrames(FILE *file)
{
    //Loop over animation frames
    for (size_t i = 0; i < anim_list.size(); i++) {
        for (size_t j = 0; j < anim_list[i].size(); j++) {
            //Write sprite index corresponding to sprite name
            uint16_t sprite_idx;
            FindSprite(anim_list[i][j].sprite_name, &sprite_idx);
            WriteU16(file, sprite_idx);
            //Write delay fields
            WriteU8(file, anim_list[i][j].delay);
            WriteU8(file, anim_list[i][j].max_delay);
            //Write scale fields
            WriteFloat(file, anim_list[i][j].x_scale);
            WriteFloat(file, anim_list[i][j].y_scale);
            //Write position fields
            WriteFloat(file, anim_list[i][j].x);
            WriteFloat(file, anim_list[i][j].y);
            //Write angle
            WriteS16(file, anim_list[i][j].angle);
            WriteS16(file, i); //Animation index
            WriteS16(file, (j + 1) % anim_list[i].size()); //Next frame
            //Write dummy field needed for matching
            WriteS16(file, 1);
        }
    }
}

void WriteImages(FILE *file)
{
    //Loop over images
    for (size_t i = 0; i < sprite_list.size(); i++) {
        for (size_t j = 0; j < sprite_list[i].images.size(); j++) {
            //Write texture ID
            WriteU16(file, sprite_list[i].images[j].texture_id);
            //Write number of palettes
            WriteU16(file, sprite_list[i].images[j].num_palettes);
            //Write image position
            WriteS16(file, sprite_list[i].images[j].x);
            WriteS16(file, sprite_list[i].images[j].y);
            //Write source position
            WriteU16(file, sprite_list[i].images[j].src_x);
            WriteU16(file, sprite_list[i].images[j].src_y);
            //Write image size
            WriteU16(file, sprite_list[i].images[j].w);
            WriteU16(file, sprite_list[i].images[j].h);
            WriteU8(file, 0); //Unknown field 1
            //Write alpha mode
            WriteU8(file, sprite_list[i].images[j].alpha_mode);
            WriteU8(file, 0); //Unknown field 2
            WriteU8(file, 0); //Unknown field 3
            //Write angle
            WriteS16(file, sprite_list[i].images[j].angle);
            //Write blend mode
            WriteU8(file, sprite_list[i].images[j].blend_mode);
            //Write bilinear flag
            WriteBool(file, sprite_list[i].images[j].bilinear);
            //Write flip flags
            WriteU8(file, sprite_list[i].images[j].flip);
            WriteU8(file, 255); //Alpha field (always 255)
            WriteU16(file, 0); //Unknown field 4
        }
    }
}

void VerifyFrameSpriteNames()
{
    //Loop over all animation frames
    for (size_t i = 0; i < anim_list.size(); i++) {
        for (size_t j = 0; j < anim_list[i].size(); j++) {
            uint16_t sprite_idx;
            if (!FindSprite(anim_list[i][j].sprite_name, &sprite_idx)) {
                //Terminate if sprite name is not found
                std::cout << "Sprite name " << anim_list[i][j].sprite_name << " not found." << std::endl;
                exit(1);
            }
        }
    }
}

void BuildSprite(std::string in_file, std::string out_file)
{
    //Read XML File
    tinyxml2::XMLDocument document;
    XMLCheck(document.LoadFile(in_file.c_str()));
    //Get root element
    tinyxml2::XMLElement *root = document.FirstChildElement("spritedata");
    if (!root) {
        //Terminate if root element is not found
        std::cout << "No root element found." << std::endl;
        exit(1);
    }
    //Parse sprite data
    ParseSprites(document, root);
    ParseAnims(document, root);
    VerifyFrameSpriteNames(); //Check data
    //Try to open output file
    FILE *file = fopen(out_file.c_str(), "wb");
    if (!file) {
        std::cout << "Failed to open " << out_file << " for writing." << std::endl;
        exit(1);
    }
    CalcSpriteBoundingRects(); //Get bounding rectangles for sprites
    //Create sprite header to write
    SpriteHeader header;
    CreateSpriteHeader(header);
    WriteSpriteHeader(file, header);
    //Write file sections
    WriteSprites(file);
    WriteAnims(file);
    WriteAnimFrames(file);
    WriteImages(file);
    //End file write
    fclose(file);
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) {
        //Write usage statement
        std::cout << "Usage: " << argv[0] << " {-d|-b} in [out]" << std::endl;
        std::cout << "-d dumps the input sprite file into an XML file" << std::endl;
        std::cout << "-b builds a sprite file from the input XML file" << std::endl;
        std::cout << "A derived name will be used for out if not provided." << std::endl;
        return 1;
    }
    //Get parameters to program
    std::string option = argv[1];
    std::string in_file = argv[2];
    std::string out_file = "";
    //Use fourth argument for output name if present
    if (argc == 4) {
        out_file = argv[3];
    }
    if (option == "-d") {
        //Generate derived name for sprite dump
        if (out_file == "") {
            std::string out_name = in_file.substr(0, in_file.find_last_of('.'));
            out_file = out_name + ".xml";
        }
        DumpSprite(in_file, out_file);
    } else if (option == "-b") {
        //Generate derived name for sprite build
        if (out_file == "") {
            std::string out_name = in_file.substr(0, in_file.find_last_of('.'));
            out_file = out_name + ".spr";
        }
        BuildSprite(in_file, out_file);
    } else {
        //Warn about invalid option
        std::cout << "Invalid option " << option << "." << std::endl;
        return 1;
    }
    //Program successful
    return 0;
}