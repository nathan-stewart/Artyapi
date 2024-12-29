#include "AudioSource.h"
#include <iostream>
using namespace std;

AudioSource::AudioSource()
{
}


AudioSource:: ~AudioSource()
{
}


AudioCapture::AudioCapture()
{
}


AudioCapture::~AudioCapture()
{
}


Signal AudioCapture::read()
{
    return Signal();
}


AudioFile::AudioFile(std::filesystem::path path)
{
    cout << "AudioFile: " << path << endl;
}

AudioFile::~AudioFile() 
{

}

Signal AudioFile::read() 
{
    return Signal();
};