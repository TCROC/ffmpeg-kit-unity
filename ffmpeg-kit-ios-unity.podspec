Pod::Spec.new do |s|  
    s.name              = "ffmpeg-kit-ios-unity"
    s.version           = "1.10"
    s.summary           = "FFmpeg Kit iOS Unity Framework"
    s.description       = "Ffmpeg for Unity"

    s.homepage          = "https://github.com/NON906/ffmpeg-kit"

    s.author            = { "NON906" => "mumeigames@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "ffmpegkit.xcframework/ios-arm64_arm64e/ffmpegkit.framework/LICENSE" }

    s.platform          = :ios
    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++', 'iconv'

    s.source            = { :http => "https://github.com/NON906/ffmpeg-kit/releases/download/v1.10-dev/ffmpeg-kit-unity-v1.10-dev-ios-xcframework.zip" }

    s.ios.deployment_target = '12.1'
    s.ios.frameworks    = 'AudioToolbox','AVFoundation','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks = 'ffmpegkit.xcframework', 'libavcodec.xcframework', 'libavdevice.xcframework', 'libavfilter.xcframework', 'libavformat.xcframework', 'libavutil.xcframework', 'libswresample.xcframework', 'libswscale.xcframework'

end  