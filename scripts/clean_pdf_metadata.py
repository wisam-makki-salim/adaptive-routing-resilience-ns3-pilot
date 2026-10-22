#!/usr/bin/env python3
"""Replace implementation-specific PDF metadata with document metadata."""

import argparse
from pathlib import Path

from pypdf import PdfReader, PdfWriter
from pypdf.generic import NameObject


def clean_pdf(source: Path, destination: Path, title: str, subject: str) -> None:
    reader = PdfReader(source)
    writer = PdfWriter()
    for page in reader.pages:
        writer.add_page(page)
    writer.metadata.clear()
    writer.add_metadata(
        {
            "/Author": "Wisam Makki Salim",
            "/Title": title,
            "/Subject": subject,
        }
    )
    writer._info.get_object().pop(NameObject("/Producer"), None)
    with destination.open("wb") as stream:
        writer.write(stream)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--title", required=True)
    parser.add_argument("--subject", required=True)
    args = parser.parse_args()
    clean_pdf(args.source, args.destination, args.title, args.subject)


if __name__ == "__main__":
    main()
