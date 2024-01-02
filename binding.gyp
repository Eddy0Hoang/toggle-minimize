{
    "targets": [
        {
            "include_dirs": [
                "<!(node -e \"require('nan')\")"
            ],
            "target_name": "toggle-minimize",
            "product_dir": "../bin",
            "sources": [],
            "conditions": [
                ['OS=="win"', {'sources':['main.cc']},  { "sources": ["none.cc"] }],
            ]
        }
    ]
}
