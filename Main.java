import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.*;

class Event {
    public String data;
}

class CachedMessageQueue {
    private final Object mutex_ = new Object();
    private final Queue<Event> cache_ = new LinkedList<>();

    public void enqueue(Event event) {
        synchronized (mutex_) {
            cache_.add(event);
            mutex_.notify();
        }
    }

    public Event dequeue() throws InterruptedException {
        synchronized (mutex_) {
            while (cache_.isEmpty()) {
                mutex_.wait();
            }
            return cache_.poll();
        }
    }
}

public class Main {
    public static void main(String[] args) {
        final int numThreads = 4;
        ExecutorService threadPool = Executors.newFixedThreadPool(numThreads);
        CachedMessageQueue messageQueue = new CachedMessageQueue();

        Thread producerThread = new Thread(() -> {
            for (int i = 1; i <= 10; ++i) {
                Event event = new Event();
                event.data = "Event " + Integer.toString(i);
                threadPool.execute(() -> messageQueue.enqueue(event));
            }
        });

        Thread consumerThread = new Thread(() -> {
            for (int i = 1; i <= 10; ++i) {
                threadPool.execute(() -> {
                    try {
                        Event event = messageQueue.dequeue();
                        System.out.println("Received event: " + event.data);
                    } catch (InterruptedException e) {
                        return;
                    }
                });
            }
        });

        producerThread.start();
        consumerThread.start();

        try {
            producerThread.join();
            consumerThread.join();
            threadPool.shutdown();
            threadPool.awaitTermination(Long.MAX_VALUE, TimeUnit.NANOSECONDS);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
