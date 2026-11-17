"""CSV loading independent of the GUI and serial connection."""
import csv
import math

from .protocol import TELEMETRY_FIELDS


def load_recording(path):
    samples = []
    with open(path, newline='', encoding='utf-8-sig') as stream:
        reader = csv.DictReader(stream)
        required = {'timestamp', *TELEMETRY_FIELDS}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError('CSV requires timestamp and all 12 telemetry columns')
        start = None
        previous = None
        for row_number, row in enumerate(reader, 2):
            try:
                timestamp = float(row['timestamp'])
                values = {name: None if row[name] == '' else float(row[name]) for name in TELEMETRY_FIELDS}
                if not all(math.isfinite(v) for v in (timestamp, *(v for v in values.values() if v is not None))):
                    raise ValueError('non-finite number')
                if previous is not None and timestamp < previous:
                    raise ValueError('timestamps must be nondecreasing')
            except (TypeError, ValueError, OverflowError) as exc:
                raise ValueError(f'Invalid CSV row {row_number}: {exc}') from exc
            if start is None:
                start = timestamp
            if not math.isfinite(timestamp - start):
                raise ValueError(f'Invalid CSV row {row_number}: time range is too large')
            previous = timestamp
            samples.append((timestamp - start, values))
    if not samples:
        raise ValueError('CSV has no telemetry samples')
    return samples
