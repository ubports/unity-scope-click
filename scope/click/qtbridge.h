/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *              Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef QT_CORE_WORLD_BRIDGE_H_
#define QT_CORE_WORLD_BRIDGE_H_

#include <QObject>

#include <functional>
#include <future>
#include <iostream>

namespace qt
{
namespace core
{
namespace world
{
// Forward declaration to allow for friend declaration in HeapAllocatedObject.
class Environment;
}
}

/**
 * @brief Models an object allocated on the heap in qt world, but pulled over
 * to this world to be passed on to tasks. Please note that compilation will
 * abort if QObject is not a base of T.
 *
 * One other important thing to note: The destrucotor will throw a std::runtime_error
 * if a HeapAllocatedObject has not been free'd in Qt world and does _not_ have a parent
 * assigned. In that case, the object would be leaked.
 *
 */
template<typename T>
class HeapAllocatedObject
{
public:
    static_assert(
            std::is_base_of<QObject, T>::value,
            "HeapAllocatedObject<T> is only required to transport QObject"
            " subclasses over the world boundary.");

    /** Constructs an empty, invalid instance. */
    HeapAllocatedObject() = default;

    /** HeapAllocatedObjects, i.e., their shared state, can be copied. */
    HeapAllocatedObject(const HeapAllocatedObject<T>& rhs) = default;

    /** HeapAllocatedObjects, i.e., their shared state can be assigned. */
    HeapAllocatedObject& operator=(const HeapAllocatedObject<T>& rhs) = default;

    /** HeapAllocatedObjects, i.e., their shared state can be compared for equality. */
    inline bool operator==(const HeapAllocatedObject<T>& rhs) const
    {
        return state == rhs.state || state->instance == rhs.state->instance;
    }

    /** Check if this object contains a valid instance. */
    inline operator bool() const
    {
        return state->instance != nullptr;
    }

private:
    friend class qt::core::world::Environment;

    struct Private
    {
        Private(T* instance) : instance(instance)
        {
        }

        ~Private()
        {
            if (instance != nullptr && instance->parent() == nullptr)
            {
                std::cerr << "HeapAllocatedObject::Private::~Private: Giving up ownership "
                             "on a QObject instance without a parent: "
                          << std::string(qPrintable(instance->metaObject()->className()))
                          << std::endl;
                ::abort();
            }
        }

        T* instance{nullptr};
    };

    HeapAllocatedObject(T* instance) : state(std::make_shared<Private>(instance))
    {
    }

    std::shared_ptr<Private> state;
};

namespace core
{
namespace world
{
/**
 * @brief The Environment class models the environment in the Qt world
 * that tasks operate under.
 */
class Environment : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Allocates subclasses of a QObject in the Qt world to make sure
     * thread assumptions are satisfied.
     *
     * @tparam Args Construction argument types to be passed to the class's c'tor.
     * @param args Construction arguments to be passed to the class's c'tor.
     */
    template<typename T, typename... Args>
    qt::HeapAllocatedObject<T> allocate(Args... args)
    {
        return qt::HeapAllocatedObject<T>{new T(args...)};
    }

    /**
     * @brief Frees a previously allocated object in the Qt world.
     * @param object The object to be freed.
     */
    template<typename T>
    void free(const qt::HeapAllocatedObject<T>& object)
    {
        delete object.state->instance;
        object.state->instance = nullptr;
    }

    /**
     * @brief Provides access to the instance contained in the object handle.
     * @param object The object containing the instance to be accessed.
     * @return A pointer to an instance of T, or nullptr.
     */
    template<typename T>
    T* resolve(const HeapAllocatedObject<T>& object)
    {
        return object.state->instance;
    }

protected:
    Environment(QObject* parent);
};

/**
 * @brief Sets up the Qt core world and executes its event loop. Blocks until destroy() is called.
 * @param argc Number of arguments in argv.
 * @param argv Array of command-line arguments.
 * @param ready Functor be called when the world has been setup and is about to be executed.
 * @throw std::runtime_error in case of errors.
 */
void build_and_run(int argc, char** argv, const std::function<void()>& ready);

/**
 * @brief Destroys the Qt core world and quits its event loop.
 */
void destroy();

/**
 * @brief Enters the Qt core world and schedules the given task for execution.
 * @param task The task to be executed in the Qt core world.
 * @return A std::future that can be waited for to synchronize to the world's internal event loop.
 */
std::future<void> enter_with_task(const std::function<void(Environment&)>& task);


/**
 * @brief Enters the Qt core world and schedules the given task for execution.
 * @param task The task to be executed in the Qt core world.
 * @return A std::future that can be waited for to get hold of the result of the task.
 */
template<typename T>
inline std::future<T> enter_with_task_and_expect_result(const std::function<T(Environment&)>& task)
{
    std::shared_ptr<std::promise<T>> promise = std::make_shared<std::promise<T>>();
    std::future<T> future = promise->get_future();

    auto t = [promise, task](Environment& env)
    {
        try
        {
            promise->set_value(task(env));
        } catch(...)
        {
            promise->set_exception(std::current_exception());
        }
    };

    enter_with_task(t);

    return future;
}
}
}
}

#endif // QT_CORE_WORLD_BRIDGE_H_
