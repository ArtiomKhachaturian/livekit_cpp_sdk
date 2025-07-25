import QtQuick
import QtQml.Models

Item {



    ObjectModel {
        id: audioTracks
    }

    function addAudioTrack(track) {
        if (null !== track) {
            audioTracks.append(track)
        }
    }
}
