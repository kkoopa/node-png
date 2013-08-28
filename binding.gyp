{
    "targets": [
        {
            "target_name": "png",
            "sources": [
                "src/common.cpp",
                "src/png_encoder.cpp",
                "src/png.cpp",
                "src/fixed_png_stack.cpp",
                "src/dynamic_png_stack.cpp",
                "src/module.cpp",
            ],
            "include_dirs" : ["<!(node -p -e \"require('path').dirname(require.resolve('nan'))\")"],
            "conditions" : [
                [
                    'OS=="linux"', {
                        "libraries" : [
                            '-lpng',
                            '-lz'
                        ],
                        'cflags!': [ '-fno-exceptions' ],
                        'cflags_cc!': [ '-fno-exceptions' ]
                    }
                ],
                [
                    'OS=="mac"', {
                        'xcode_settings': {
                            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
                        },
                        "libraries" : [
                            '<!@(pkg-config libpng --libs)'
                        ]
                    }
                ],
                [
                    'OS=="win"', {
                        "include_dirs" : [ "gyp/include" ],
                        "libraries" : [
                            '<(module_root_dir)/gyp/lib/libpng.lib',
                            '<(module_root_dir)/gyp/lib/zlib.lib'
                        ]
                    }
                ]
            ]
        }
    ]
}
