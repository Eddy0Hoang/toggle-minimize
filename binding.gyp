{
    "targets": [
        {
            "include_dirs": [
                "<!(node -e \"require('nan')\")"
            ],
            "target_name": "toggle-minimize",
            "product_dir": "../out",
            "sources": [],
            "conditions": [
                ['OS=="win"', {'sources':['main.cc']},  { "sources": ["none.cc"] }],
            ]
        }
    ]
}
