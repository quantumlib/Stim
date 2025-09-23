class Logger:
    def __init__(self, filename, logging=True, stdout_print=False):
        self.filename = filename
        self.logging = logging
        self.stdout_print = stdout_print
        # Overwrite the file at initialization
        with open(self.filename, 'w'):
            pass  # Just truncate the file

    def print(self, message):
        if self.logging:
            with open(self.filename, 'a') as f:
                f.write(str(message) + '\n')
        if self.stdout_print:
            print(message)
