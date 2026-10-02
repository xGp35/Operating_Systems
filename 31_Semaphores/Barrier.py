class Barrier:
    def __init__(self, n):
        self.n = n
        self.count = 0
        self.mutex = Semaphore(1)
        self.turnstile = Semaphore(0)
        self.turnstile2 = Semaphore(0)

    def phase1(self):
        self.mutex.wait()
        self.count += 1
        if self.count == self.n:
            self.turnstile.signal(self.n)
        self.mutex.signal()
        self.turnstile.wait()
    
    def phase2(self):
        self.mutex.wait()
        self.count -= 1
        if self.count == 0:
            self.turnstile2.signal(self.n)
        self.mutex.signal()
        self.turnstile2.wait()
    
    def wait(self):
        self.phase1()
        self.phase2()